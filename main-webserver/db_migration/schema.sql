--
-- PostgreSQL database dump
--

-- Dumped from database version 13.3 (Ubuntu 13.3-1.pgdg20.04+1)
-- Dumped by pg_dump version 13.3

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: hardware; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA hardware;


--
-- Name: hdb_catalog; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA hdb_catalog;


--
-- Name: logging; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA logging;


--
-- Name: sales; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA sales;


--
-- Name: pg_stat_statements; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pg_stat_statements WITH SCHEMA public;


--
-- Name: EXTENSION pg_stat_statements; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION pg_stat_statements IS 'track planning and execution statistics of all SQL statements executed';


--
-- Name: pgcrypto; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pgcrypto WITH SCHEMA public;


--
-- Name: EXTENSION pgcrypto; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION pgcrypto IS 'cryptographic functions';


--
-- Name: change_trigger(); Type: FUNCTION; Schema: hardware; Owner: -
--

CREATE FUNCTION hardware.change_trigger() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
       BEGIN
         IF TG_OP = 'INSERT'
         THEN INSERT INTO logging.t_instance_history (
                tabname, schemaname, operation, new_val
              ) VALUES (
                TG_RELNAME, TG_TABLE_SCHEMA, TG_OP, row_to_json(NEW)
              );
           RETURN NEW;
         ELSIF  TG_OP = 'UPDATE' AND NEW.status <> OLD.status
         THEN
           INSERT INTO logging.t_instance_history (
             tabname, schemaname, operation, new_val, old_val
           )
           VALUES (TG_RELNAME, TG_TABLE_SCHEMA, TG_OP, row_to_json(NEW),
row_to_json(OLD));
           RETURN NEW;
         ELSIF  TG_OP = 'UPDATE'
         THEN
           RETURN NEW;
         ELSIF TG_OP = 'DELETE'
         THEN
           INSERT INTO logging.t_instance_history
             (tabname, schemaname, operation, old_val)
             VALUES (
               TG_RELNAME, TG_TABLE_SCHEMA, TG_OP, row_to_json(OLD)
             );
             RETURN OLD;
         END IF;
       END;
$$;


--
-- Name: change_trigger_regions(); Type: FUNCTION; Schema: hardware; Owner: -
--

CREATE FUNCTION hardware.change_trigger_regions() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
       BEGIN
         IF TG_OP = 'INSERT'
         THEN INSERT INTO logging.t_region_history (
                tabname, schemaname, operation, new_val
              ) VALUES (
                TG_RELNAME, TG_TABLE_SCHEMA, TG_OP, row_to_json(NEW)
              );
           RETURN NEW;
         ELSIF  TG_OP = 'UPDATE'
         THEN
           INSERT INTO logging.t_region_history (
             tabname, schemaname, operation, new_val, old_val
           )
           VALUES (TG_RELNAME, TG_TABLE_SCHEMA, TG_OP, row_to_json(NEW),
row_to_json(OLD));
           RETURN NEW;
         ELSIF TG_OP = 'DELETE'
         THEN
           INSERT INTO logging.t_region_history
             (tabname, schemaname, operation, old_val)
             VALUES (
               TG_RELNAME, TG_TABLE_SCHEMA, TG_OP, row_to_json(OLD)
             );
             RETURN OLD;
         END IF;
       END;
$$;


--
-- Name: gen_hasura_uuid(); Type: FUNCTION; Schema: hdb_catalog; Owner: -
--

CREATE FUNCTION hdb_catalog.gen_hasura_uuid() RETURNS uuid
    LANGUAGE sql
    AS $$select gen_random_uuid()$$;


SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: t_region_history; Type: TABLE; Schema: logging; Owner: -
--

CREATE TABLE logging.t_region_history (
    id integer NOT NULL,
    tstamp timestamp without time zone DEFAULT now(),
    schemaname text,
    tabname text,
    operation text,
    who text DEFAULT CURRENT_USER,
    new_val json,
    old_val json
);


--
-- Name: ami_status_changes; Type: VIEW; Schema: hardware; Owner: -
--

CREATE VIEW hardware.ami_status_changes AS
 SELECT t_region_history.tstamp,
    (t_region_history.new_val ->> 'ami_id'::text) AS ami_changed,
    COALESCE((t_region_history.new_val ->> 'ami_active'::text), 'deleted'::text) AS new_status,
    COALESCE((t_region_history.old_val ->> 'ami_active'::text), 'newly added'::text) AS old_status
   FROM logging.t_region_history
  WHERE ((t_region_history.old_val IS NULL) OR (t_region_history.new_val IS NULL) OR ((t_region_history.old_val ->> 'ami_active'::text) <> (t_region_history.new_val ->> 'ami_active'::text)));


--
-- Name: instance_info; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.instance_info (
    instance_name character varying NOT NULL,
    cloud_provider_id character varying NOT NULL,
    creation_time_utc_unix_ms bigint NOT NULL,
    memory_remaining_kb bigint DEFAULT 2000 NOT NULL,
    nanocpus_remaining bigint DEFAULT 1024 NOT NULL,
    gpu_vram_remaining_kb bigint DEFAULT 1024 NOT NULL,
    mandelbox_capacity bigint DEFAULT 0 NOT NULL,
    last_updated_utc_unix_ms bigint DEFAULT '-1'::integer NOT NULL,
    ip character varying NOT NULL,
    aws_ami_id character varying NOT NULL,
    location character varying NOT NULL,
    status character varying NOT NULL,
    commit_hash character varying NOT NULL,
    aws_instance_type character varying NOT NULL
);


--
-- Name: mandelbox_info; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.mandelbox_info (
    mandelbox_id character varying NOT NULL,
    user_id character varying NOT NULL,
    instance_name character varying NOT NULL,
    status character varying NOT NULL,
    creation_time_utc_unix_ms bigint NOT NULL
);


--
-- Name: instances_with_room_for_mandelboxes; Type: VIEW; Schema: hardware; Owner: -
--

CREATE VIEW hardware.instances_with_room_for_mandelboxes AS
 SELECT sub_with_running.instance_name,
    sub_with_running.aws_ami_id,
    sub_with_running.commit_hash,
    sub_with_running.location,
    sub_with_running.status,
    sub_with_running.mandelbox_capacity,
    sub_with_running.num_running_mandelboxes
   FROM ( SELECT base_table.instance_name,
            base_table.aws_ami_id,
            base_table.location,
            base_table.commit_hash,
            base_table.status,
            base_table.mandelbox_capacity,
            COALESCE(base_table.count, (0)::bigint) AS num_running_mandelboxes
           FROM (( SELECT instance_info.instance_name,
                    instance_info.aws_ami_id,
                    instance_info.location,
                    instance_info.commit_hash,
                    instance_info.status,
                    instance_info.mandelbox_capacity
                   FROM hardware.instance_info
                  WHERE (((instance_info.status)::text <> 'DRAINING'::text) AND ((instance_info.status)::text <> 'HOST_SERVICE_UNRESPONSIVE'::text))) instances
             LEFT JOIN ( SELECT count(*) AS count,
                    mandelbox_info.instance_name AS cont_inst
                   FROM hardware.mandelbox_info
                  GROUP BY mandelbox_info.instance_name) mandelboxes ON (((instances.instance_name)::text = (mandelboxes.cont_inst)::text))) base_table) sub_with_running
  WHERE (sub_with_running.num_running_mandelboxes < sub_with_running.mandelbox_capacity)
  ORDER BY sub_with_running.location, sub_with_running.num_running_mandelboxes DESC;


--
-- Name: t_instance_history; Type: TABLE; Schema: logging; Owner: -
--

CREATE TABLE logging.t_instance_history (
    id integer NOT NULL,
    tstamp timestamp without time zone DEFAULT now(),
    schemaname text,
    tabname text,
    operation text,
    who text DEFAULT CURRENT_USER,
    new_val json,
    old_val json
);


--
-- Name: instance_status_changes; Type: VIEW; Schema: hardware; Owner: -
--

CREATE VIEW hardware.instance_status_changes AS
 SELECT t_instance_history.tstamp AS "timestamp",
    COALESCE((t_instance_history.new_val ->> 'instance_name'::text), (t_instance_history.old_val ->> 'instance_name'::text)) AS instance_name,
    COALESCE((t_instance_history.new_val ->> 'status'::text), 'deleted'::text) AS new_status,
    COALESCE((t_instance_history.old_val ->> 'status'::text), 'newly added'::text) AS old_status
   FROM logging.t_instance_history
  WHERE ((t_instance_history.new_val IS NULL) OR (t_instance_history.old_val IS NULL) OR ((t_instance_history.new_val ->> 'status'::text) <> (t_instance_history.old_val ->> 'status'::text)))
  ORDER BY t_instance_history.tstamp;


--
-- Name: lingering_instances; Type: VIEW; Schema: hardware; Owner: -
--

CREATE VIEW hardware.lingering_instances AS
 SELECT instance_info.instance_name,
    instance_info.cloud_provider_id,
    instance_info.status
   FROM hardware.instance_info
  WHERE ((((((date_part('epoch'::text, now()) * (1000)::double precision))::bigint - instance_info.last_updated_utc_unix_ms) > 120000) AND ((instance_info.status)::text <> 'PRE_CONNECTION'::text)) OR (((((date_part('epoch'::text, now()) * (1000)::double precision))::bigint - instance_info.last_updated_utc_unix_ms) > 900000) AND ((((date_part('epoch'::text, now()) * (1000)::double precision))::bigint - instance_info.creation_time_utc_unix_ms) > 900000) AND ((instance_info.status)::text <> 'DRAINING'::text) AND ((instance_info.status)::text <> 'HOST_SERVICE_UNRESPONSIVE'::text)));


--
-- Name: region_to_ami; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.region_to_ami (
    region_name character varying NOT NULL,
    ami_id character varying NOT NULL,
    client_commit_hash character varying NOT NULL,
    ami_active boolean DEFAULT false NOT NULL,
    protected_from_scale_down boolean DEFAULT false NOT NULL
);


--
-- Name: supported_app_images; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.supported_app_images (
    app_id character varying NOT NULL,
    logo_url character varying,
    category character varying,
    description character varying,
    long_description character varying,
    url character varying,
    tos character varying,
    active boolean NOT NULL,
    preboot_number double precision DEFAULT 0.0 NOT NULL
);


--
-- Name: hdb_action_log; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_action_log (
    id uuid DEFAULT hdb_catalog.gen_hasura_uuid() NOT NULL,
    action_name text,
    input_payload jsonb NOT NULL,
    request_headers jsonb NOT NULL,
    session_variables jsonb NOT NULL,
    response_payload jsonb,
    errors jsonb,
    created_at timestamp with time zone DEFAULT now() NOT NULL,
    response_received_at timestamp with time zone,
    status text NOT NULL,
    CONSTRAINT hdb_action_log_status_check CHECK ((status = ANY (ARRAY['created'::text, 'processing'::text, 'completed'::text, 'error'::text])))
);


--
-- Name: hdb_cron_event_invocation_logs; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_cron_event_invocation_logs (
    id text DEFAULT hdb_catalog.gen_hasura_uuid() NOT NULL,
    event_id text,
    status integer,
    request json,
    response json,
    created_at timestamp with time zone DEFAULT now()
);


--
-- Name: hdb_cron_events; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_cron_events (
    id text DEFAULT hdb_catalog.gen_hasura_uuid() NOT NULL,
    trigger_name text NOT NULL,
    scheduled_time timestamp with time zone NOT NULL,
    status text DEFAULT 'scheduled'::text NOT NULL,
    tries integer DEFAULT 0 NOT NULL,
    created_at timestamp with time zone DEFAULT now(),
    next_retry_at timestamp with time zone,
    CONSTRAINT valid_status CHECK ((status = ANY (ARRAY['scheduled'::text, 'locked'::text, 'delivered'::text, 'error'::text, 'dead'::text])))
);


--
-- Name: hdb_metadata; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_metadata (
    id integer NOT NULL,
    metadata json NOT NULL,
    resource_version integer DEFAULT 1 NOT NULL
);


--
-- Name: hdb_scheduled_event_invocation_logs; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_scheduled_event_invocation_logs (
    id text DEFAULT hdb_catalog.gen_hasura_uuid() NOT NULL,
    event_id text,
    status integer,
    request json,
    response json,
    created_at timestamp with time zone DEFAULT now()
);


--
-- Name: hdb_scheduled_events; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_scheduled_events (
    id text DEFAULT hdb_catalog.gen_hasura_uuid() NOT NULL,
    webhook_conf json NOT NULL,
    scheduled_time timestamp with time zone NOT NULL,
    retry_conf json,
    payload json,
    header_conf json,
    status text DEFAULT 'scheduled'::text NOT NULL,
    tries integer DEFAULT 0 NOT NULL,
    created_at timestamp with time zone DEFAULT now(),
    next_retry_at timestamp with time zone,
    comment text,
    CONSTRAINT valid_status CHECK ((status = ANY (ARRAY['scheduled'::text, 'locked'::text, 'delivered'::text, 'error'::text, 'dead'::text])))
);


--
-- Name: hdb_schema_notifications; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_schema_notifications (
    id integer NOT NULL,
    notification json NOT NULL,
    resource_version integer DEFAULT 1 NOT NULL,
    instance_id uuid NOT NULL,
    updated_at timestamp with time zone DEFAULT now(),
    CONSTRAINT hdb_schema_notifications_id_check CHECK ((id = 1))
);


--
-- Name: hdb_version; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_version (
    hasura_uuid uuid DEFAULT hdb_catalog.gen_hasura_uuid() NOT NULL,
    version text NOT NULL,
    upgraded_on timestamp with time zone NOT NULL,
    cli_state jsonb DEFAULT '{}'::jsonb NOT NULL,
    console_state jsonb DEFAULT '{}'::jsonb NOT NULL
);


--
-- Name: instance_status_change; Type: VIEW; Schema: logging; Owner: -
--

CREATE VIEW logging.instance_status_change AS
 SELECT t_instance_history.tstamp,
    t_instance_history.old_val,
    t_instance_history.new_val
   FROM logging.t_instance_history
  WHERE ((t_instance_history.old_val ->> 'status'::text) <> (t_instance_history.new_val ->> 'status'::text));


--
-- Name: t_instance_history_id_seq; Type: SEQUENCE; Schema: logging; Owner: -
--

CREATE SEQUENCE logging.t_instance_history_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: t_instance_history_id_seq; Type: SEQUENCE OWNED BY; Schema: logging; Owner: -
--

ALTER SEQUENCE logging.t_instance_history_id_seq OWNED BY logging.t_instance_history.id;


--
-- Name: t_region_history_id_seq; Type: SEQUENCE; Schema: logging; Owner: -
--

CREATE SEQUENCE logging.t_region_history_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- Name: t_region_history_id_seq; Type: SEQUENCE OWNED BY; Schema: logging; Owner: -
--

ALTER SEQUENCE logging.t_region_history_id_seq OWNED BY logging.t_region_history.id;


--
-- Name: email_templates; Type: TABLE; Schema: sales; Owner: -
--

CREATE TABLE sales.email_templates (
    id character varying NOT NULL,
    url character varying NOT NULL,
    title text
);


--
-- Name: t_instance_history id; Type: DEFAULT; Schema: logging; Owner: -
--

ALTER TABLE ONLY logging.t_instance_history ALTER COLUMN id SET DEFAULT nextval('logging.t_instance_history_id_seq'::regclass);


--
-- Name: t_region_history id; Type: DEFAULT; Schema: logging; Owner: -
--

ALTER TABLE ONLY logging.t_region_history ALTER COLUMN id SET DEFAULT nextval('logging.t_region_history_id_seq'::regclass);


--
-- Name: region_to_ami _region_name_ami_id_unique_constraint; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.region_to_ami
    ADD CONSTRAINT _region_name_ami_id_unique_constraint UNIQUE (region_name, ami_id);


--
-- Name: instance_info instance_info_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.instance_info
    ADD CONSTRAINT instance_info_pkey PRIMARY KEY (instance_name);


--
-- Name: mandelbox_info mandelbox_info_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.mandelbox_info
    ADD CONSTRAINT mandelbox_info_pkey PRIMARY KEY (mandelbox_id);


--
-- Name: region_to_ami region_to_ami_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.region_to_ami
    ADD CONSTRAINT region_to_ami_pkey PRIMARY KEY (region_name, client_commit_hash);


--
-- Name: supported_app_images supported_app_images_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.supported_app_images
    ADD CONSTRAINT supported_app_images_pkey PRIMARY KEY (app_id);


--
-- Name: hdb_action_log hdb_action_log_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_action_log
    ADD CONSTRAINT hdb_action_log_pkey PRIMARY KEY (id);


--
-- Name: hdb_cron_event_invocation_logs hdb_cron_event_invocation_logs_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_cron_event_invocation_logs
    ADD CONSTRAINT hdb_cron_event_invocation_logs_pkey PRIMARY KEY (id);


--
-- Name: hdb_cron_events hdb_cron_events_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_cron_events
    ADD CONSTRAINT hdb_cron_events_pkey PRIMARY KEY (id);


--
-- Name: hdb_metadata hdb_metadata_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_metadata
    ADD CONSTRAINT hdb_metadata_pkey PRIMARY KEY (id);


--
-- Name: hdb_metadata hdb_metadata_resource_version_key; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_metadata
    ADD CONSTRAINT hdb_metadata_resource_version_key UNIQUE (resource_version);


--
-- Name: hdb_scheduled_event_invocation_logs hdb_scheduled_event_invocation_logs_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_scheduled_event_invocation_logs
    ADD CONSTRAINT hdb_scheduled_event_invocation_logs_pkey PRIMARY KEY (id);


--
-- Name: hdb_scheduled_events hdb_scheduled_events_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_scheduled_events
    ADD CONSTRAINT hdb_scheduled_events_pkey PRIMARY KEY (id);


--
-- Name: hdb_schema_notifications hdb_schema_notifications_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_schema_notifications
    ADD CONSTRAINT hdb_schema_notifications_pkey PRIMARY KEY (id);


--
-- Name: hdb_version hdb_version_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_version
    ADD CONSTRAINT hdb_version_pkey PRIMARY KEY (hasura_uuid);


--
-- Name: email_templates email_templates_pkey; Type: CONSTRAINT; Schema: sales; Owner: -
--

ALTER TABLE ONLY sales.email_templates
    ADD CONSTRAINT email_templates_pkey PRIMARY KEY (id);


--
-- Name: hdb_cron_event_status; Type: INDEX; Schema: hdb_catalog; Owner: -
--

CREATE INDEX hdb_cron_event_status ON hdb_catalog.hdb_cron_events USING btree (status);


--
-- Name: hdb_cron_events_unique_scheduled; Type: INDEX; Schema: hdb_catalog; Owner: -
--

CREATE UNIQUE INDEX hdb_cron_events_unique_scheduled ON hdb_catalog.hdb_cron_events USING btree (trigger_name, scheduled_time) WHERE (status = 'scheduled'::text);


--
-- Name: hdb_scheduled_event_status; Type: INDEX; Schema: hdb_catalog; Owner: -
--

CREATE INDEX hdb_scheduled_event_status ON hdb_catalog.hdb_scheduled_events USING btree (status);


--
-- Name: hdb_version_one_row; Type: INDEX; Schema: hdb_catalog; Owner: -
--

CREATE UNIQUE INDEX hdb_version_one_row ON hdb_catalog.hdb_version USING btree (((version IS NOT NULL)));


--
-- Name: instance_info t; Type: TRIGGER; Schema: hardware; Owner: -
--

CREATE TRIGGER t BEFORE INSERT OR DELETE OR UPDATE ON hardware.instance_info FOR EACH ROW EXECUTE FUNCTION hardware.change_trigger();


--
-- Name: region_to_ami t; Type: TRIGGER; Schema: hardware; Owner: -
--

CREATE TRIGGER t BEFORE INSERT OR DELETE OR UPDATE ON hardware.region_to_ami FOR EACH ROW EXECUTE FUNCTION hardware.change_trigger_regions();


--
-- Name: mandelbox_info instance_name_fk; Type: FK CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.mandelbox_info
    ADD CONSTRAINT instance_name_fk FOREIGN KEY (instance_name) REFERENCES hardware.instance_info(instance_name) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: hdb_cron_event_invocation_logs hdb_cron_event_invocation_logs_event_id_fkey; Type: FK CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_cron_event_invocation_logs
    ADD CONSTRAINT hdb_cron_event_invocation_logs_event_id_fkey FOREIGN KEY (event_id) REFERENCES hdb_catalog.hdb_cron_events(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: hdb_scheduled_event_invocation_logs hdb_scheduled_event_invocation_logs_event_id_fkey; Type: FK CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_scheduled_event_invocation_logs
    ADD CONSTRAINT hdb_scheduled_event_invocation_logs_event_id_fkey FOREIGN KEY (event_id) REFERENCES hdb_catalog.hdb_scheduled_events(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- PostgreSQL database dump complete
--
