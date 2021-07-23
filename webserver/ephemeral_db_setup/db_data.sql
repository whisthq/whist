--
-- PostgreSQL database dump
--

-- Dumped from database version 12.7 (Ubuntu 12.7-1.pgdg16.04+1)
-- Dumped by pg_dump version 12.7 (Debian 12.7-1.pgdg100+1)

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
-- Data for Name: region_to_ami; Type: TABLE DATA; Schema: hardware; Owner: uap4ch2emueqo9
--


INSERT INTO hardware.region_to_ami (region_name, ami_id, client_commit_hash, ami_active) VALUES ('eu-central-1', 'ami-04e791bf07624272e', 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, client_commit_hash, ami_active) VALUES ('eu-west-1', 'ami-04b85bdd938419795', 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, client_commit_hash, ami_active) VALUES ('us-east-1', 'ami-00c40082600650a9a', 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, client_commit_hash, ami_active) VALUES ('us-east-1', 'ami-00c40082600650a9b', 'dummy_client_hash_2', false);
INSERT INTO hardware.region_to_ami (region_name, ami_id, client_commit_hash, ami_active) VALUES ('us-east-2', 'ami-0a7da7479f37c924a', 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, client_commit_hash, ami_active) VALUES ('us-east-2', 'ami-0a7da7479f37c924b', 'dummy_client_hash_2', false);
INSERT INTO hardware.region_to_ami (region_name, ami_id, client_commit_hash, ami_active) VALUES ('us-west-1', 'ami-0bb9ea3cf997fb4ec', 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, client_commit_hash, ami_active) VALUES ('us-west-2', 'ami-0a2e1892cafee19d4', 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, client_commit_hash, ami_active) VALUES ('ca-central-1', 'ami-09b2b490cdf85f1f1', 'dummy_client_hash', true);


--
-- PostgreSQL database dump complete
--
