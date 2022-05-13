--
-- PostgreSQL database dump
--

-- Dumped from database version 13.4 (Debian 13.4-1.pgdg100+1)
-- Dumped by pg_dump version 13.4 (Debian 13.4-0+deb11u1)

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
-- Name: whist; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA whist;


--
-- Name: set_current_timestamp_updated_at(); Type: FUNCTION; Schema: whist; Owner: -
--

CREATE FUNCTION whist.set_current_timestamp_updated_at() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
DECLARE
  _new record;
BEGIN
  _new := NEW;
  _new."updated_at" = NOW();
  RETURN _new;
END;
$$;


SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: images; Type: TABLE; Schema: whist; Owner: -
--

CREATE TABLE whist.images (
    provider character varying NOT NULL,
    region character varying NOT NULL,
    image_id character varying NOT NULL,
    client_sha character varying NOT NULL,
    updated_at timestamp with time zone DEFAULT now() NOT NULL
);


--
-- Name: instances; Type: TABLE; Schema: whist; Owner: -
--

CREATE TABLE whist.instances (
    id character varying NOT NULL,
    provider character varying NOT NULL,
    region character varying NOT NULL,
    image_id character varying NOT NULL,
    client_sha character varying NOT NULL,
    ip_addr inet,
    instance_type character varying NOT NULL,
    remaining_capacity integer NOT NULL,
    status character varying NOT NULL,
    created_at timestamp with time zone DEFAULT now() NOT NULL,
    updated_at timestamp with time zone DEFAULT now() NOT NULL
);


--
-- Name: mandelboxes; Type: TABLE; Schema: whist; Owner: -
--

CREATE TABLE whist.mandelboxes (
    id character varying NOT NULL,
    app character varying NOT NULL,
    instance_id character varying NOT NULL,
    user_id character varying,
    session_id character varying,
    status character varying NOT NULL,
    created_at timestamp with time zone DEFAULT now() NOT NULL,
    updated_at timestamp with time zone DEFAULT now() NOT NULL
);


--
-- Name: images images_pkey; Type: CONSTRAINT; Schema: whist; Owner: -
--

ALTER TABLE ONLY whist.images
    ADD CONSTRAINT images_pkey PRIMARY KEY (provider, region);


--
-- Name: instances instances_pkey; Type: CONSTRAINT; Schema: whist; Owner: -
--

ALTER TABLE ONLY whist.instances
    ADD CONSTRAINT instances_pkey PRIMARY KEY (id);


--
-- Name: mandelboxes mandelboxes_pkey; Type: CONSTRAINT; Schema: whist; Owner: -
--

ALTER TABLE ONLY whist.mandelboxes
    ADD CONSTRAINT mandelboxes_pkey PRIMARY KEY (id);


--
-- Name: images set_whist_images_updated_at; Type: TRIGGER; Schema: whist; Owner: -
--

CREATE TRIGGER set_whist_images_updated_at BEFORE UPDATE ON whist.images FOR EACH ROW EXECUTE FUNCTION whist.set_current_timestamp_updated_at();


--
-- Name: instances set_whist_instances_updated_at; Type: TRIGGER; Schema: whist; Owner: -
--

CREATE TRIGGER set_whist_instances_updated_at BEFORE UPDATE ON whist.instances FOR EACH ROW EXECUTE FUNCTION whist.set_current_timestamp_updated_at();


--
-- Name: mandelboxes mandelboxes_instance_id_fkey; Type: FK CONSTRAINT; Schema: whist; Owner: -
--

ALTER TABLE ONLY whist.mandelboxes
    ADD CONSTRAINT mandelboxes_instance_id_fkey FOREIGN KEY (instance_id) REFERENCES whist.instances(id) ON UPDATE RESTRICT ON DELETE RESTRICT;


--
-- PostgreSQL database dump complete
--

