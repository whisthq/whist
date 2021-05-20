--
-- PostgreSQL database dump
--

-- Dumped from database version 12.5 (Debian 12.5-1.pgdg100+1)
-- Dumped by pg_dump version 13.2

-- Started on 2021-05-20 07:54:07

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
-- TOC entry 7 (class 2615 OID 16386)
-- Name: hardware; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA hardware;


--
-- TOC entry 9 (class 2615 OID 16387)
-- Name: hdb_catalog; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA hdb_catalog;


--
-- TOC entry 8 (class 2615 OID 16388)
-- Name: hdb_pro_catalog; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA hdb_pro_catalog;


--
-- TOC entry 5 (class 2615 OID 16389)
-- Name: hdb_views; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA hdb_views;


--
-- TOC entry 12 (class 2615 OID 16390)
-- Name: sales; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA sales;


--
-- TOC entry 250 (class 1255 OID 16393)
-- Name: check_violation(text); Type: FUNCTION; Schema: hdb_catalog; Owner: -
--

CREATE FUNCTION hdb_catalog.check_violation(msg text) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
  BEGIN
    RAISE check_violation USING message=msg;
  END;
$$;


--
-- TOC entry 251 (class 1255 OID 16394)
-- Name: event_trigger_table_name_update(); Type: FUNCTION; Schema: hdb_catalog; Owner: -
--

CREATE FUNCTION hdb_catalog.event_trigger_table_name_update() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
BEGIN
  IF (NEW.table_schema, NEW.table_name) <> (OLD.table_schema, OLD.table_name)  THEN
    UPDATE hdb_catalog.event_triggers
    SET schema_name = NEW.table_schema, table_name = NEW.table_name
    WHERE (schema_name, table_name) = (OLD.table_schema, OLD.table_name);
  END IF;
  RETURN NEW;
END;
$$;


--
-- TOC entry 252 (class 1255 OID 16395)
-- Name: hdb_schema_update_event_notifier(); Type: FUNCTION; Schema: hdb_catalog; Owner: -
--

CREATE FUNCTION hdb_catalog.hdb_schema_update_event_notifier() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
  DECLARE
    instance_id uuid;
    occurred_at timestamptz;
    invalidations json;
    curr_rec record;
  BEGIN
    instance_id = NEW.instance_id;
    occurred_at = NEW.occurred_at;
    invalidations = NEW.invalidations;
    PERFORM pg_notify('hasura_schema_update', json_build_object(
      'instance_id', instance_id,
      'occurred_at', occurred_at,
      'invalidations', invalidations
      )::text);
    RETURN curr_rec;
  END;
$$;


--
-- TOC entry 253 (class 1255 OID 16396)
-- Name: inject_table_defaults(text, text, text, text); Type: FUNCTION; Schema: hdb_catalog; Owner: -
--

CREATE FUNCTION hdb_catalog.inject_table_defaults(view_schema text, view_name text, tab_schema text, tab_name text) RETURNS void
    LANGUAGE plpgsql
    AS $$
    DECLARE
        r RECORD;
    BEGIN
      FOR r IN SELECT column_name, column_default FROM information_schema.columns WHERE table_schema = tab_schema AND table_name = tab_name AND column_default IS NOT NULL LOOP
          EXECUTE format('ALTER VIEW %I.%I ALTER COLUMN %I SET DEFAULT %s;', view_schema, view_name, r.column_name, r.column_default);
      END LOOP;
    END;
$$;


--
-- TOC entry 254 (class 1255 OID 16397)
-- Name: insert_event_log(text, text, text, text, json); Type: FUNCTION; Schema: hdb_catalog; Owner: -
--

CREATE FUNCTION hdb_catalog.insert_event_log(schema_name text, table_name text, trigger_name text, op text, row_data json) RETURNS text
    LANGUAGE plpgsql
    AS $$
  DECLARE
    id text;
    payload json;
    session_variables json;
    server_version_num int;
    trace_context json;
  BEGIN
    id := gen_random_uuid();
    server_version_num := current_setting('server_version_num');
    IF server_version_num >= 90600 THEN
      session_variables := current_setting('hasura.user', 't');
      trace_context := current_setting('hasura.tracecontext', 't');
    ELSE
      BEGIN
        session_variables := current_setting('hasura.user');
      EXCEPTION WHEN OTHERS THEN
                  session_variables := NULL;
      END;
      BEGIN
        trace_context := current_setting('hasura.tracecontext');
      EXCEPTION WHEN OTHERS THEN
        trace_context := NULL;
      END;
    END IF;
    payload := json_build_object(
      'op', op,
      'data', row_data,
      'session_variables', session_variables,
      'trace_context', trace_context
    );
    INSERT INTO hdb_catalog.event_log
                (id, schema_name, table_name, trigger_name, payload)
    VALUES
    (id, schema_name, table_name, trigger_name, payload);
    RETURN id;
  END;
$$;


SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- TOC entry 207 (class 1259 OID 16398)
-- Name: cluster_info; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.cluster_info (
    cluster character varying NOT NULL,
    "maxCPURemainingPerInstance" double precision,
    "maxMemoryRemainingPerInstance" double precision,
    "pendingTasksCount" bigint,
    "runningTasksCount" bigint,
    "registeredContainerInstancesCount" bigint,
    "minContainers" bigint,
    "maxContainers" bigint,
    status character varying,
    location character varying
);


--
-- TOC entry 209 (class 1259 OID 16410)
-- Name: cluster_sorted; Type: VIEW; Schema: hardware; Owner: -
--

CREATE VIEW hardware.cluster_sorted AS
 SELECT cluster_info.cluster,
    cluster_info."maxCPURemainingPerInstance",
    cluster_info."maxMemoryRemainingPerInstance",
    cluster_info."pendingTasksCount",
    cluster_info."runningTasksCount",
    cluster_info."registeredContainerInstancesCount",
    cluster_info."minContainers",
    cluster_info."maxContainers",
    cluster_info.status,
    cluster_info.location
   FROM hardware.cluster_info
  WHERE (((cluster_info."registeredContainerInstancesCount" < cluster_info."maxContainers") OR (COALESCE(cluster_info."maxMemoryRemainingPerInstance", (0)::double precision) > (8500)::double precision) OR (cluster_info."maxMemoryRemainingPerInstance" = (0)::double precision)) AND ((cluster_info.cluster)::text !~~ '%test%'::text))
  ORDER BY cluster_info."registeredContainerInstancesCount" DESC, COALESCE(cluster_info."runningTasksCount", (0)::bigint) DESC, cluster_info."maxCPURemainingPerInstance" DESC, cluster_info."maxMemoryRemainingPerInstance" DESC;


--
-- TOC entry 248 (class 1259 OID 16786)
-- Name: container_info; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.container_info (
    container_id bigint NOT NULL,
    user_id character varying,
    instance_id character varying NOT NULL,
    status character varying
);


--
-- TOC entry 247 (class 1259 OID 16784)
-- Name: container_info_container_id_seq; Type: SEQUENCE; Schema: hardware; Owner: -
--

CREATE SEQUENCE hardware.container_info_container_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- TOC entry 3221 (class 0 OID 0)
-- Dependencies: 247
-- Name: container_info_container_id_seq; Type: SEQUENCE OWNED BY; Schema: hardware; Owner: -
--

ALTER SEQUENCE hardware.container_info_container_id_seq OWNED BY hardware.container_info.container_id;


--
-- TOC entry 208 (class 1259 OID 16404)
-- Name: instance_info; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.instance_info (
    instance_id character varying NOT NULL,
    auth_token character varying NOT NULL,
    "lastHeartbeated" double precision,
    "memoryRemainingInInstanceInMb" bigint,
    "CPURemainingInInstance" bigint,
    "GPURemainingInInstance" bigint,
    "maxContainers" bigint,
    last_pinged bigint,
    ip character varying NOT NULL,
    ami_id character varying NOT NULL,
    location character varying NOT NULL,
    instance_type character varying NOT NULL
);


--
-- TOC entry 249 (class 1259 OID 16800)
-- Name: instance_sorted; Type: VIEW; Schema: hardware; Owner: -
--

CREATE VIEW hardware.instance_sorted AS
 SELECT sub_with_running.instance_id,
    sub_with_running.instance_type,
    sub_with_running."maxContainers",
    sub_with_running.running_containers
   FROM ( SELECT base_table.instance_id,
            base_table.instance_type,
            base_table."maxContainers",
            COALESCE(base_table.count, (0)::bigint) AS running_containers
           FROM (( SELECT instance_info.instance_id,
                    instance_info.instance_type,
                    instance_info."maxContainers"
                   FROM hardware.instance_info) instances
             LEFT JOIN ( SELECT count(*) AS count,
                    container_info.instance_id AS cont_inst
                   FROM hardware.container_info
                  GROUP BY container_info.instance_id) containers ON (((instances.instance_id)::text = (containers.cont_inst)::text))) base_table) sub_with_running
  WHERE (sub_with_running.running_containers < sub_with_running."maxContainers");


--
-- TOC entry 210 (class 1259 OID 16415)
-- Name: region_to_ami; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.region_to_ami (
    region_name character varying NOT NULL,
    ami_id character varying NOT NULL,
    allowed boolean DEFAULT true NOT NULL,
    region_being_updated boolean DEFAULT false NOT NULL
);


--
-- TOC entry 211 (class 1259 OID 16423)
-- Name: supported_app_images; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.supported_app_images (
    app_id character varying NOT NULL,
    logo_url character varying,
    task_definition character varying,
    task_version integer,
    category character varying,
    description character varying,
    long_description character varying,
    url character varying,
    tos character varying,
    active boolean NOT NULL,
    preboot_number double precision DEFAULT 0.0 NOT NULL
);


--
-- TOC entry 212 (class 1259 OID 16430)
-- Name: user_app_state; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.user_app_state (
    user_id character varying(255),
    ip character varying(255),
    client_app_auth_secret character varying(255),
    port integer,
    task_id character varying(255) NOT NULL,
    state character varying(255)
);


--
-- TOC entry 213 (class 1259 OID 16436)
-- Name: user_containers; Type: TABLE; Schema: hardware; Owner: -
--

CREATE TABLE hardware.user_containers (
    container_id character varying NOT NULL,
    ip character varying NOT NULL,
    location character varying NOT NULL,
    state character varying NOT NULL,
    user_id character varying,
    port_32262 bigint DEFAULT '-1'::integer NOT NULL,
    last_pinged bigint,
    cluster character varying,
    port_32263 bigint DEFAULT '-1'::integer NOT NULL,
    port_32273 bigint DEFAULT '-1'::integer NOT NULL,
    secret_key text NOT NULL,
    task_definition character varying,
    task_version integer,
    dpi integer DEFAULT 96
);


--
-- TOC entry 214 (class 1259 OID 16452)
-- Name: event_triggers; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.event_triggers (
    name text NOT NULL,
    type text NOT NULL,
    schema_name text NOT NULL,
    table_name text NOT NULL,
    configuration json,
    comment text
);


--
-- TOC entry 215 (class 1259 OID 16458)
-- Name: hdb_action; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_action (
    action_name text NOT NULL,
    action_defn jsonb NOT NULL,
    comment text,
    is_system_defined boolean DEFAULT false
);


--
-- TOC entry 216 (class 1259 OID 16468)
-- Name: hdb_action_permission; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_action_permission (
    action_name text NOT NULL,
    role_name text NOT NULL,
    definition jsonb DEFAULT '{}'::jsonb NOT NULL,
    comment text
);


--
-- TOC entry 217 (class 1259 OID 16475)
-- Name: hdb_allowlist; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_allowlist (
    collection_name text
);


--
-- TOC entry 218 (class 1259 OID 16481)
-- Name: hdb_check_constraint; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_check_constraint AS
 SELECT (n.nspname)::text AS table_schema,
    (ct.relname)::text AS table_name,
    (r.conname)::text AS constraint_name,
    pg_get_constraintdef(r.oid, true) AS "check"
   FROM ((pg_constraint r
     JOIN pg_class ct ON ((r.conrelid = ct.oid)))
     JOIN pg_namespace n ON ((ct.relnamespace = n.oid)))
  WHERE (r.contype = 'c'::"char");


--
-- TOC entry 219 (class 1259 OID 16486)
-- Name: hdb_computed_field; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_computed_field (
    table_schema text NOT NULL,
    table_name text NOT NULL,
    computed_field_name text NOT NULL,
    definition jsonb NOT NULL,
    comment text
);


--
-- TOC entry 220 (class 1259 OID 16492)
-- Name: hdb_computed_field_function; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_computed_field_function AS
 SELECT hdb_computed_field.table_schema,
    hdb_computed_field.table_name,
    hdb_computed_field.computed_field_name,
        CASE
            WHEN (((hdb_computed_field.definition -> 'function'::text) ->> 'name'::text) IS NULL) THEN (hdb_computed_field.definition ->> 'function'::text)
            ELSE ((hdb_computed_field.definition -> 'function'::text) ->> 'name'::text)
        END AS function_name,
        CASE
            WHEN (((hdb_computed_field.definition -> 'function'::text) ->> 'schema'::text) IS NULL) THEN 'public'::text
            ELSE ((hdb_computed_field.definition -> 'function'::text) ->> 'schema'::text)
        END AS function_schema
   FROM hdb_catalog.hdb_computed_field;


--
-- TOC entry 221 (class 1259 OID 16502)
-- Name: hdb_cron_triggers; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_cron_triggers (
    name text NOT NULL,
    webhook_conf json NOT NULL,
    cron_schedule text NOT NULL,
    payload json,
    retry_conf json,
    header_conf json,
    include_in_metadata boolean DEFAULT false NOT NULL,
    comment text
);


--
-- TOC entry 222 (class 1259 OID 16509)
-- Name: hdb_custom_types; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_custom_types (
    custom_types jsonb NOT NULL
);


--
-- TOC entry 223 (class 1259 OID 16515)
-- Name: hdb_foreign_key_constraint; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_foreign_key_constraint AS
 SELECT (q.table_schema)::text AS table_schema,
    (q.table_name)::text AS table_name,
    (q.constraint_name)::text AS constraint_name,
    (min(q.constraint_oid))::integer AS constraint_oid,
    min((q.ref_table_table_schema)::text) AS ref_table_table_schema,
    min((q.ref_table)::text) AS ref_table,
    json_object_agg(ac.attname, afc.attname) AS column_mapping,
    min((q.confupdtype)::text) AS on_update,
    min((q.confdeltype)::text) AS on_delete,
    json_agg(ac.attname) AS columns,
    json_agg(afc.attname) AS ref_columns
   FROM ((( SELECT ctn.nspname AS table_schema,
            ct.relname AS table_name,
            r.conrelid AS table_id,
            r.conname AS constraint_name,
            r.oid AS constraint_oid,
            cftn.nspname AS ref_table_table_schema,
            cft.relname AS ref_table,
            r.confrelid AS ref_table_id,
            r.confupdtype,
            r.confdeltype,
            unnest(r.conkey) AS column_id,
            unnest(r.confkey) AS ref_column_id
           FROM ((((pg_constraint r
             JOIN pg_class ct ON ((r.conrelid = ct.oid)))
             JOIN pg_namespace ctn ON ((ct.relnamespace = ctn.oid)))
             JOIN pg_class cft ON ((r.confrelid = cft.oid)))
             JOIN pg_namespace cftn ON ((cft.relnamespace = cftn.oid)))
          WHERE (r.contype = 'f'::"char")) q
     JOIN pg_attribute ac ON (((q.column_id = ac.attnum) AND (q.table_id = ac.attrelid))))
     JOIN pg_attribute afc ON (((q.ref_column_id = afc.attnum) AND (q.ref_table_id = afc.attrelid))))
  GROUP BY q.table_schema, q.table_name, q.constraint_name;


--
-- TOC entry 224 (class 1259 OID 16520)
-- Name: hdb_function; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_function (
    function_schema text NOT NULL,
    function_name text NOT NULL,
    configuration jsonb DEFAULT '{}'::jsonb NOT NULL,
    is_system_defined boolean DEFAULT false
);


--
-- TOC entry 225 (class 1259 OID 16528)
-- Name: hdb_function_agg; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_function_agg AS
 SELECT (p.proname)::text AS function_name,
    (pn.nspname)::text AS function_schema,
    pd.description,
        CASE
            WHEN (p.provariadic = (0)::oid) THEN false
            ELSE true
        END AS has_variadic,
        CASE
            WHEN ((p.provolatile)::text = ('i'::character(1))::text) THEN 'IMMUTABLE'::text
            WHEN ((p.provolatile)::text = ('s'::character(1))::text) THEN 'STABLE'::text
            WHEN ((p.provolatile)::text = ('v'::character(1))::text) THEN 'VOLATILE'::text
            ELSE NULL::text
        END AS function_type,
    pg_get_functiondef(p.oid) AS function_definition,
    (rtn.nspname)::text AS return_type_schema,
    (rt.typname)::text AS return_type_name,
    (rt.typtype)::text AS return_type_type,
    p.proretset AS returns_set,
    ( SELECT COALESCE(json_agg(json_build_object('schema', q.schema, 'name', q.name, 'type', q.type)), '[]'::json) AS "coalesce"
           FROM ( SELECT pt.typname AS name,
                    pns.nspname AS schema,
                    pt.typtype AS type,
                    pat.ordinality
                   FROM ((unnest(COALESCE(p.proallargtypes, (p.proargtypes)::oid[])) WITH ORDINALITY pat(oid, ordinality)
                     LEFT JOIN pg_type pt ON ((pt.oid = pat.oid)))
                     LEFT JOIN pg_namespace pns ON ((pt.typnamespace = pns.oid)))
                  ORDER BY pat.ordinality) q) AS input_arg_types,
    to_json(COALESCE(p.proargnames, ARRAY[]::text[])) AS input_arg_names,
    p.pronargdefaults AS default_args,
    (p.oid)::integer AS function_oid
   FROM ((((pg_proc p
     JOIN pg_namespace pn ON ((pn.oid = p.pronamespace)))
     JOIN pg_type rt ON ((rt.oid = p.prorettype)))
     JOIN pg_namespace rtn ON ((rtn.oid = rt.typnamespace)))
     LEFT JOIN pg_description pd ON ((p.oid = pd.objoid)))
  WHERE (((pn.nspname)::text !~~ 'pg_%'::text) AND ((pn.nspname)::text <> ALL (ARRAY['information_schema'::text, 'hdb_catalog'::text, 'hdb_views'::text])) AND (NOT (EXISTS ( SELECT 1
           FROM pg_aggregate
          WHERE ((pg_aggregate.aggfnoid)::oid = p.oid)))));


--
-- TOC entry 226 (class 1259 OID 16533)
-- Name: hdb_function_info_agg; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_function_info_agg AS
 SELECT hdb_function_agg.function_name,
    hdb_function_agg.function_schema,
    row_to_json(( SELECT e.*::record AS e
           FROM ( SELECT hdb_function_agg.description,
                    hdb_function_agg.has_variadic,
                    hdb_function_agg.function_type,
                    hdb_function_agg.return_type_schema,
                    hdb_function_agg.return_type_name,
                    hdb_function_agg.return_type_type,
                    hdb_function_agg.returns_set,
                    hdb_function_agg.input_arg_types,
                    hdb_function_agg.input_arg_names,
                    hdb_function_agg.default_args,
                    ((EXISTS ( SELECT 1
                           FROM information_schema.tables
                          WHERE (((tables.table_schema)::name = hdb_function_agg.return_type_schema) AND ((tables.table_name)::name = hdb_function_agg.return_type_name)))) OR (EXISTS ( SELECT 1
                           FROM pg_matviews
                          WHERE ((pg_matviews.schemaname = hdb_function_agg.return_type_schema) AND (pg_matviews.matviewname = hdb_function_agg.return_type_name))))) AS returns_table) e)) AS function_info
   FROM hdb_catalog.hdb_function_agg;


--
-- TOC entry 227 (class 1259 OID 16538)
-- Name: hdb_permission; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_permission (
    table_schema name NOT NULL,
    table_name name NOT NULL,
    role_name text NOT NULL,
    perm_type text NOT NULL,
    perm_def jsonb NOT NULL,
    comment text,
    is_system_defined boolean DEFAULT false,
    CONSTRAINT hdb_permission_perm_type_check CHECK ((perm_type = ANY (ARRAY['insert'::text, 'select'::text, 'update'::text, 'delete'::text])))
);


--
-- TOC entry 228 (class 1259 OID 16546)
-- Name: hdb_permission_agg; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_permission_agg AS
 SELECT hdb_permission.table_schema,
    hdb_permission.table_name,
    hdb_permission.role_name,
    json_object_agg(hdb_permission.perm_type, hdb_permission.perm_def) AS permissions
   FROM hdb_catalog.hdb_permission
  GROUP BY hdb_permission.table_schema, hdb_permission.table_name, hdb_permission.role_name;


--
-- TOC entry 229 (class 1259 OID 16550)
-- Name: hdb_primary_key; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_primary_key AS
 SELECT tc.table_schema,
    tc.table_name,
    tc.constraint_name,
    json_agg(constraint_column_usage.column_name) AS columns
   FROM (information_schema.table_constraints tc
     JOIN ( SELECT x.tblschema AS table_schema,
            x.tblname AS table_name,
            x.colname AS column_name,
            x.cstrname AS constraint_name
           FROM ( SELECT DISTINCT nr.nspname,
                    r.relname,
                    a.attname,
                    c.conname
                   FROM pg_namespace nr,
                    pg_class r,
                    pg_attribute a,
                    pg_depend d,
                    pg_namespace nc,
                    pg_constraint c
                  WHERE ((nr.oid = r.relnamespace) AND (r.oid = a.attrelid) AND (d.refclassid = ('pg_class'::regclass)::oid) AND (d.refobjid = r.oid) AND (d.refobjsubid = a.attnum) AND (d.classid = ('pg_constraint'::regclass)::oid) AND (d.objid = c.oid) AND (c.connamespace = nc.oid) AND (c.contype = 'c'::"char") AND (r.relkind = ANY (ARRAY['r'::"char", 'p'::"char"])) AND (NOT a.attisdropped))
                UNION ALL
                 SELECT nr.nspname,
                    r.relname,
                    a.attname,
                    c.conname
                   FROM pg_namespace nr,
                    pg_class r,
                    pg_attribute a,
                    pg_namespace nc,
                    pg_constraint c
                  WHERE ((nr.oid = r.relnamespace) AND (r.oid = a.attrelid) AND (nc.oid = c.connamespace) AND (r.oid =
                        CASE c.contype
                            WHEN 'f'::"char" THEN c.confrelid
                            ELSE c.conrelid
                        END) AND (a.attnum = ANY (
                        CASE c.contype
                            WHEN 'f'::"char" THEN c.confkey
                            ELSE c.conkey
                        END)) AND (NOT a.attisdropped) AND (c.contype = ANY (ARRAY['p'::"char", 'u'::"char", 'f'::"char"])) AND (r.relkind = ANY (ARRAY['r'::"char", 'p'::"char"])))) x(tblschema, tblname, colname, cstrname)) constraint_column_usage ON ((((tc.constraint_name)::text = (constraint_column_usage.constraint_name)::text) AND ((tc.table_schema)::text = (constraint_column_usage.table_schema)::text) AND ((tc.table_name)::text = (constraint_column_usage.table_name)::text))))
  WHERE ((tc.constraint_type)::text = 'PRIMARY KEY'::text)
  GROUP BY tc.table_schema, tc.table_name, tc.constraint_name;


--
-- TOC entry 230 (class 1259 OID 16555)
-- Name: hdb_query_collection; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_query_collection (
    collection_name text NOT NULL,
    collection_defn jsonb NOT NULL,
    comment text,
    is_system_defined boolean DEFAULT false
);


--
-- TOC entry 231 (class 1259 OID 16562)
-- Name: hdb_relationship; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_relationship (
    table_schema name NOT NULL,
    table_name name NOT NULL,
    rel_name text NOT NULL,
    rel_type text,
    rel_def jsonb NOT NULL,
    comment text,
    is_system_defined boolean DEFAULT false,
    CONSTRAINT hdb_relationship_rel_type_check CHECK ((rel_type = ANY (ARRAY['object'::text, 'array'::text])))
);


--
-- TOC entry 232 (class 1259 OID 16570)
-- Name: hdb_remote_relationship; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_remote_relationship (
    remote_relationship_name text NOT NULL,
    table_schema name NOT NULL,
    table_name name NOT NULL,
    definition jsonb NOT NULL
);


--
-- TOC entry 233 (class 1259 OID 16576)
-- Name: hdb_role; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_role AS
 SELECT DISTINCT q.role_name
   FROM ( SELECT hdb_permission.role_name
           FROM hdb_catalog.hdb_permission
        UNION ALL
         SELECT hdb_action_permission.role_name
           FROM hdb_catalog.hdb_action_permission) q;


--
-- TOC entry 234 (class 1259 OID 16586)
-- Name: hdb_schema_update_event; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_schema_update_event (
    instance_id uuid NOT NULL,
    occurred_at timestamp with time zone DEFAULT now() NOT NULL,
    invalidations json NOT NULL
);


--
-- TOC entry 235 (class 1259 OID 16593)
-- Name: hdb_table; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.hdb_table (
    table_schema name NOT NULL,
    table_name name NOT NULL,
    configuration jsonb,
    is_system_defined boolean DEFAULT false,
    is_enum boolean DEFAULT false NOT NULL
);


--
-- TOC entry 236 (class 1259 OID 16601)
-- Name: hdb_table_info_agg; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_table_info_agg AS
 SELECT schema.nspname AS table_schema,
    "table".relname AS table_name,
    jsonb_build_object('oid', ("table".oid)::integer, 'columns', COALESCE(columns.info, '[]'::jsonb), 'primary_key', primary_key.info, 'unique_constraints', COALESCE(unique_constraints.info, '[]'::jsonb), 'foreign_keys', COALESCE(foreign_key_constraints.info, '[]'::jsonb), 'view_info',
        CASE "table".relkind
            WHEN 'v'::"char" THEN jsonb_build_object('is_updatable', ((pg_relation_is_updatable(("table".oid)::regclass, true) & 4) = 4), 'is_insertable', ((pg_relation_is_updatable(("table".oid)::regclass, true) & 8) = 8), 'is_deletable', ((pg_relation_is_updatable(("table".oid)::regclass, true) & 16) = 16))
            ELSE NULL::jsonb
        END, 'description', description.description) AS info
   FROM ((((((pg_class "table"
     JOIN pg_namespace schema ON ((schema.oid = "table".relnamespace)))
     LEFT JOIN pg_description description ON (((description.classoid = ('pg_class'::regclass)::oid) AND (description.objoid = "table".oid) AND (description.objsubid = 0))))
     LEFT JOIN LATERAL ( SELECT jsonb_agg(jsonb_build_object('name', "column".attname, 'position', "column".attnum, 'type', COALESCE(base_type.typname, type.typname), 'is_nullable', (NOT "column".attnotnull), 'description', col_description("table".oid, ("column".attnum)::integer))) AS info
           FROM ((pg_attribute "column"
             LEFT JOIN pg_type type ON ((type.oid = "column".atttypid)))
             LEFT JOIN pg_type base_type ON (((type.typtype = 'd'::"char") AND (base_type.oid = type.typbasetype))))
          WHERE (("column".attrelid = "table".oid) AND ("column".attnum > 0) AND (NOT "column".attisdropped))) columns ON (true))
     LEFT JOIN LATERAL ( SELECT jsonb_build_object('constraint', jsonb_build_object('name', class.relname, 'oid', (class.oid)::integer), 'columns', COALESCE(columns_1.info, '[]'::jsonb)) AS info
           FROM ((pg_index index
             JOIN pg_class class ON ((class.oid = index.indexrelid)))
             LEFT JOIN LATERAL ( SELECT jsonb_agg("column".attname) AS info
                   FROM pg_attribute "column"
                  WHERE (("column".attrelid = "table".oid) AND ("column".attnum = ANY ((index.indkey)::smallint[])))) columns_1 ON (true))
          WHERE ((index.indrelid = "table".oid) AND index.indisprimary)) primary_key ON (true))
     LEFT JOIN LATERAL ( SELECT jsonb_agg(jsonb_build_object('name', class.relname, 'oid', (class.oid)::integer)) AS info
           FROM (pg_index index
             JOIN pg_class class ON ((class.oid = index.indexrelid)))
          WHERE ((index.indrelid = "table".oid) AND index.indisunique AND (NOT index.indisprimary))) unique_constraints ON (true))
     LEFT JOIN LATERAL ( SELECT jsonb_agg(jsonb_build_object('constraint', jsonb_build_object('name', foreign_key.constraint_name, 'oid', foreign_key.constraint_oid), 'columns', foreign_key.columns, 'foreign_table', jsonb_build_object('schema', foreign_key.ref_table_table_schema, 'name', foreign_key.ref_table), 'foreign_columns', foreign_key.ref_columns)) AS info
           FROM hdb_catalog.hdb_foreign_key_constraint foreign_key
          WHERE ((foreign_key.table_schema = schema.nspname) AND (foreign_key.table_name = "table".relname))) foreign_key_constraints ON (true))
  WHERE ("table".relkind = ANY (ARRAY['r'::"char", 't'::"char", 'v'::"char", 'm'::"char", 'f'::"char", 'p'::"char"]));


--
-- TOC entry 237 (class 1259 OID 16606)
-- Name: hdb_unique_constraint; Type: VIEW; Schema: hdb_catalog; Owner: -
--

CREATE VIEW hdb_catalog.hdb_unique_constraint AS
 SELECT tc.table_name,
    tc.constraint_schema AS table_schema,
    tc.constraint_name,
    json_agg(kcu.column_name) AS columns
   FROM (information_schema.table_constraints tc
     JOIN information_schema.key_column_usage kcu USING (constraint_schema, constraint_name))
  WHERE ((tc.constraint_type)::text = 'UNIQUE'::text)
  GROUP BY tc.table_name, tc.constraint_schema, tc.constraint_name;


--
-- TOC entry 238 (class 1259 OID 16614)
-- Name: migration_settings; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.migration_settings (
    setting text NOT NULL,
    value text NOT NULL
);


--
-- TOC entry 239 (class 1259 OID 16620)
-- Name: remote_schemas; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.remote_schemas (
    id bigint NOT NULL,
    name text,
    definition json,
    comment text
);


--
-- TOC entry 240 (class 1259 OID 16626)
-- Name: remote_schemas_id_seq; Type: SEQUENCE; Schema: hdb_catalog; Owner: -
--

CREATE SEQUENCE hdb_catalog.remote_schemas_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- TOC entry 3222 (class 0 OID 0)
-- Dependencies: 240
-- Name: remote_schemas_id_seq; Type: SEQUENCE OWNED BY; Schema: hdb_catalog; Owner: -
--

ALTER SEQUENCE hdb_catalog.remote_schemas_id_seq OWNED BY hdb_catalog.remote_schemas.id;


--
-- TOC entry 241 (class 1259 OID 16628)
-- Name: schema_migrations; Type: TABLE; Schema: hdb_catalog; Owner: -
--

CREATE TABLE hdb_catalog.schema_migrations (
    version bigint NOT NULL,
    dirty boolean NOT NULL
);


--
-- TOC entry 242 (class 1259 OID 16631)
-- Name: hdb_instances_ref; Type: TABLE; Schema: hdb_pro_catalog; Owner: -
--

CREATE TABLE hdb_pro_catalog.hdb_instances_ref (
    instance_id uuid,
    heartbeat_timestamp timestamp without time zone
);


--
-- TOC entry 243 (class 1259 OID 16634)
-- Name: hdb_pro_config; Type: TABLE; Schema: hdb_pro_catalog; Owner: -
--

CREATE TABLE hdb_pro_catalog.hdb_pro_config (
    id integer NOT NULL,
    pro_config jsonb,
    last_updated timestamp without time zone DEFAULT now()
);


--
-- TOC entry 244 (class 1259 OID 16641)
-- Name: hdb_pro_state; Type: TABLE; Schema: hdb_pro_catalog; Owner: -
--

CREATE TABLE hdb_pro_catalog.hdb_pro_state (
    id integer NOT NULL,
    hasura_pro_state jsonb,
    schema_version integer NOT NULL,
    data_version bigint NOT NULL,
    last_updated timestamp without time zone DEFAULT now()
);


--
-- TOC entry 245 (class 1259 OID 16648)
-- Name: users; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.users (
    user_id character varying(250) NOT NULL,
    encrypted_config_token character varying(256) DEFAULT ''::character varying NOT NULL,
    token character varying(250),
    name character varying(250),
    password character varying(250) NOT NULL,
    stripe_customer_id character varying(250),
    reason_for_signup text,
    verified boolean DEFAULT false,
    created_at timestamp with time zone DEFAULT CURRENT_TIMESTAMP NOT NULL
);


--
-- TOC entry 246 (class 1259 OID 16657)
-- Name: email_templates; Type: TABLE; Schema: sales; Owner: -
--

CREATE TABLE sales.email_templates (
    id character varying NOT NULL,
    url character varying NOT NULL,
    title text
);


--
-- TOC entry 3002 (class 2604 OID 16789)
-- Name: container_info container_id; Type: DEFAULT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.container_info ALTER COLUMN container_id SET DEFAULT nextval('hardware.container_info_container_id_seq'::regclass);


--
-- TOC entry 2996 (class 2604 OID 16663)
-- Name: remote_schemas id; Type: DEFAULT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.remote_schemas ALTER COLUMN id SET DEFAULT nextval('hdb_catalog.remote_schemas_id_seq'::regclass);


--
-- TOC entry 3004 (class 2606 OID 16665)
-- Name: cluster_info cluster_info_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.cluster_info
    ADD CONSTRAINT cluster_info_pkey PRIMARY KEY (cluster);


--
-- TOC entry 3065 (class 2606 OID 16794)
-- Name: container_info container_info_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.container_info
    ADD CONSTRAINT container_info_pkey PRIMARY KEY (container_id);


--
-- TOC entry 3006 (class 2606 OID 16783)
-- Name: instance_info instance_info_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.instance_info
    ADD CONSTRAINT instance_info_pkey PRIMARY KEY (instance_id);


--
-- TOC entry 3008 (class 2606 OID 16667)
-- Name: region_to_ami region_to_ami_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.region_to_ami
    ADD CONSTRAINT region_to_ami_pkey PRIMARY KEY (region_name);


--
-- TOC entry 3010 (class 2606 OID 16669)
-- Name: supported_app_images supported_app_images_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.supported_app_images
    ADD CONSTRAINT supported_app_images_pkey PRIMARY KEY (app_id);


--
-- TOC entry 3012 (class 2606 OID 16671)
-- Name: supported_app_images unique_taskdef; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.supported_app_images
    ADD CONSTRAINT unique_taskdef UNIQUE (task_definition);


--
-- TOC entry 3014 (class 2606 OID 16673)
-- Name: user_app_state user_app_state_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.user_app_state
    ADD CONSTRAINT user_app_state_pkey PRIMARY KEY (task_id);


--
-- TOC entry 3020 (class 2606 OID 16675)
-- Name: user_containers user_containers_pkey; Type: CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.user_containers
    ADD CONSTRAINT user_containers_pkey PRIMARY KEY (container_id);


--
-- TOC entry 3022 (class 2606 OID 16677)
-- Name: event_triggers event_triggers_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.event_triggers
    ADD CONSTRAINT event_triggers_pkey PRIMARY KEY (name);


--
-- TOC entry 3026 (class 2606 OID 16679)
-- Name: hdb_action_permission hdb_action_permission_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_action_permission
    ADD CONSTRAINT hdb_action_permission_pkey PRIMARY KEY (action_name, role_name);


--
-- TOC entry 3024 (class 2606 OID 16681)
-- Name: hdb_action hdb_action_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_action
    ADD CONSTRAINT hdb_action_pkey PRIMARY KEY (action_name);


--
-- TOC entry 3028 (class 2606 OID 16683)
-- Name: hdb_allowlist hdb_allowlist_collection_name_key; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_allowlist
    ADD CONSTRAINT hdb_allowlist_collection_name_key UNIQUE (collection_name);


--
-- TOC entry 3030 (class 2606 OID 16685)
-- Name: hdb_computed_field hdb_computed_field_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_computed_field
    ADD CONSTRAINT hdb_computed_field_pkey PRIMARY KEY (table_schema, table_name, computed_field_name);


--
-- TOC entry 3032 (class 2606 OID 16687)
-- Name: hdb_cron_triggers hdb_cron_triggers_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_cron_triggers
    ADD CONSTRAINT hdb_cron_triggers_pkey PRIMARY KEY (name);


--
-- TOC entry 3034 (class 2606 OID 16689)
-- Name: hdb_function hdb_function_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_function
    ADD CONSTRAINT hdb_function_pkey PRIMARY KEY (function_schema, function_name);


--
-- TOC entry 3036 (class 2606 OID 16691)
-- Name: hdb_permission hdb_permission_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_permission
    ADD CONSTRAINT hdb_permission_pkey PRIMARY KEY (table_schema, table_name, role_name, perm_type);


--
-- TOC entry 3038 (class 2606 OID 16693)
-- Name: hdb_query_collection hdb_query_collection_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_query_collection
    ADD CONSTRAINT hdb_query_collection_pkey PRIMARY KEY (collection_name);


--
-- TOC entry 3040 (class 2606 OID 16695)
-- Name: hdb_relationship hdb_relationship_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_relationship
    ADD CONSTRAINT hdb_relationship_pkey PRIMARY KEY (table_schema, table_name, rel_name);


--
-- TOC entry 3042 (class 2606 OID 16697)
-- Name: hdb_remote_relationship hdb_remote_relationship_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_remote_relationship
    ADD CONSTRAINT hdb_remote_relationship_pkey PRIMARY KEY (remote_relationship_name, table_schema, table_name);


--
-- TOC entry 3045 (class 2606 OID 16699)
-- Name: hdb_table hdb_table_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_table
    ADD CONSTRAINT hdb_table_pkey PRIMARY KEY (table_schema, table_name);


--
-- TOC entry 3047 (class 2606 OID 16701)
-- Name: migration_settings migration_settings_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.migration_settings
    ADD CONSTRAINT migration_settings_pkey PRIMARY KEY (setting);


--
-- TOC entry 3049 (class 2606 OID 16703)
-- Name: remote_schemas remote_schemas_name_key; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.remote_schemas
    ADD CONSTRAINT remote_schemas_name_key UNIQUE (name);


--
-- TOC entry 3051 (class 2606 OID 16705)
-- Name: remote_schemas remote_schemas_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.remote_schemas
    ADD CONSTRAINT remote_schemas_pkey PRIMARY KEY (id);


--
-- TOC entry 3053 (class 2606 OID 16707)
-- Name: schema_migrations schema_migrations_pkey; Type: CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.schema_migrations
    ADD CONSTRAINT schema_migrations_pkey PRIMARY KEY (version);


--
-- TOC entry 3055 (class 2606 OID 16709)
-- Name: hdb_pro_config hdb_pro_config_pkey; Type: CONSTRAINT; Schema: hdb_pro_catalog; Owner: -
--

ALTER TABLE ONLY hdb_pro_catalog.hdb_pro_config
    ADD CONSTRAINT hdb_pro_config_pkey PRIMARY KEY (id);


--
-- TOC entry 3057 (class 2606 OID 16711)
-- Name: hdb_pro_state hdb_pro_state_pkey; Type: CONSTRAINT; Schema: hdb_pro_catalog; Owner: -
--

ALTER TABLE ONLY hdb_pro_catalog.hdb_pro_state
    ADD CONSTRAINT hdb_pro_state_pkey PRIMARY KEY (id);


--
-- TOC entry 3059 (class 2606 OID 16713)
-- Name: users PK_users; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.users
    ADD CONSTRAINT "PK_users" PRIMARY KEY (user_id);


--
-- TOC entry 3061 (class 2606 OID 16715)
-- Name: users unique_user_id; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.users
    ADD CONSTRAINT unique_user_id UNIQUE (user_id);


--
-- TOC entry 3063 (class 2606 OID 16717)
-- Name: email_templates email_templates_pkey; Type: CONSTRAINT; Schema: sales; Owner: -
--

ALTER TABLE ONLY sales.email_templates
    ADD CONSTRAINT email_templates_pkey PRIMARY KEY (id);


--
-- TOC entry 3015 (class 1259 OID 16718)
-- Name: fki_app_id_fk; Type: INDEX; Schema: hardware; Owner: -
--

CREATE INDEX fki_app_id_fk ON hardware.user_containers USING btree (task_definition);


--
-- TOC entry 3016 (class 1259 OID 16719)
-- Name: fki_cluster_name_fk; Type: INDEX; Schema: hardware; Owner: -
--

CREATE INDEX fki_cluster_name_fk ON hardware.user_containers USING btree (cluster);


--
-- TOC entry 3017 (class 1259 OID 16720)
-- Name: ip_and_port; Type: INDEX; Schema: hardware; Owner: -
--

CREATE INDEX ip_and_port ON hardware.user_containers USING btree (ip, port_32262);


--
-- TOC entry 3018 (class 1259 OID 16721)
-- Name: loc_taskdef_uid; Type: INDEX; Schema: hardware; Owner: -
--

CREATE INDEX loc_taskdef_uid ON hardware.user_containers USING btree (location, task_definition, user_id);


--
-- TOC entry 3043 (class 1259 OID 16722)
-- Name: hdb_schema_update_event_one_row; Type: INDEX; Schema: hdb_catalog; Owner: -
--

CREATE UNIQUE INDEX hdb_schema_update_event_one_row ON hdb_catalog.hdb_schema_update_event USING btree (((occurred_at IS NOT NULL)));


--
-- TOC entry 3077 (class 2620 OID 16723)
-- Name: hdb_table event_trigger_table_name_update_trigger; Type: TRIGGER; Schema: hdb_catalog; Owner: -
--

CREATE TRIGGER event_trigger_table_name_update_trigger AFTER UPDATE ON hdb_catalog.hdb_table FOR EACH ROW EXECUTE FUNCTION hdb_catalog.event_trigger_table_name_update();


--
-- TOC entry 3076 (class 2620 OID 16724)
-- Name: hdb_schema_update_event hdb_schema_update_event_notifier; Type: TRIGGER; Schema: hdb_catalog; Owner: -
--

CREATE TRIGGER hdb_schema_update_event_notifier AFTER INSERT OR UPDATE ON hdb_catalog.hdb_schema_update_event FOR EACH ROW EXECUTE FUNCTION hdb_catalog.hdb_schema_update_event_notifier();


--
-- TOC entry 3075 (class 2606 OID 16795)
-- Name: container_info instance_id_fk; Type: FK CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.container_info
    ADD CONSTRAINT instance_id_fk FOREIGN KEY (instance_id) REFERENCES hardware.instance_info(instance_id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- TOC entry 3066 (class 2606 OID 16725)
-- Name: user_containers task_definition_fk; Type: FK CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.user_containers
    ADD CONSTRAINT task_definition_fk FOREIGN KEY (task_definition) REFERENCES hardware.supported_app_images(task_definition);


--
-- TOC entry 3067 (class 2606 OID 16730)
-- Name: user_containers user_containers_cluster_fkey; Type: FK CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.user_containers
    ADD CONSTRAINT user_containers_cluster_fkey FOREIGN KEY (cluster) REFERENCES hardware.cluster_info(cluster) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- TOC entry 3068 (class 2606 OID 16735)
-- Name: user_containers user_id_fk; Type: FK CONSTRAINT; Schema: hardware; Owner: -
--

ALTER TABLE ONLY hardware.user_containers
    ADD CONSTRAINT user_id_fk FOREIGN KEY (user_id) REFERENCES public.users(user_id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- TOC entry 3069 (class 2606 OID 16740)
-- Name: hdb_action_permission hdb_action_permission_action_name_fkey; Type: FK CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_action_permission
    ADD CONSTRAINT hdb_action_permission_action_name_fkey FOREIGN KEY (action_name) REFERENCES hdb_catalog.hdb_action(action_name) ON UPDATE CASCADE;


--
-- TOC entry 3070 (class 2606 OID 16745)
-- Name: hdb_allowlist hdb_allowlist_collection_name_fkey; Type: FK CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_allowlist
    ADD CONSTRAINT hdb_allowlist_collection_name_fkey FOREIGN KEY (collection_name) REFERENCES hdb_catalog.hdb_query_collection(collection_name);


--
-- TOC entry 3071 (class 2606 OID 16750)
-- Name: hdb_computed_field hdb_computed_field_table_schema_table_name_fkey; Type: FK CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_computed_field
    ADD CONSTRAINT hdb_computed_field_table_schema_table_name_fkey FOREIGN KEY (table_schema, table_name) REFERENCES hdb_catalog.hdb_table(table_schema, table_name) ON UPDATE CASCADE;


--
-- TOC entry 3072 (class 2606 OID 16755)
-- Name: hdb_permission hdb_permission_table_schema_table_name_fkey; Type: FK CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_permission
    ADD CONSTRAINT hdb_permission_table_schema_table_name_fkey FOREIGN KEY (table_schema, table_name) REFERENCES hdb_catalog.hdb_table(table_schema, table_name) ON UPDATE CASCADE;


--
-- TOC entry 3073 (class 2606 OID 16760)
-- Name: hdb_relationship hdb_relationship_table_schema_table_name_fkey; Type: FK CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_relationship
    ADD CONSTRAINT hdb_relationship_table_schema_table_name_fkey FOREIGN KEY (table_schema, table_name) REFERENCES hdb_catalog.hdb_table(table_schema, table_name) ON UPDATE CASCADE;


--
-- TOC entry 3074 (class 2606 OID 16765)
-- Name: hdb_remote_relationship hdb_remote_relationship_table_schema_table_name_fkey; Type: FK CONSTRAINT; Schema: hdb_catalog; Owner: -
--

ALTER TABLE ONLY hdb_catalog.hdb_remote_relationship
    ADD CONSTRAINT hdb_remote_relationship_table_schema_table_name_fkey FOREIGN KEY (table_schema, table_name) REFERENCES hdb_catalog.hdb_table(table_schema, table_name) ON UPDATE CASCADE;


-- Completed on 2021-05-20 07:54:09

--
-- PostgreSQL database dump complete
--
