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
-- Data for Name: images; Type: TABLE DATA; Schema: whist; Owner: -
--

INSERT INTO whist.images (provider, region, image_id, client_sha) VALUES ('AWS', 'eu-central-1', 'ami-04e791bf07624272e', 'dummy_client_hash');
INSERT INTO whist.images (provider, region, image_id, client_sha) VALUES ('AWS', 'eu-west-1', 'ami-04b85bdd938419795', 'dummy_client_hash');
INSERT INTO whist.images (provider, region, image_id, client_sha) VALUES ('AWS', 'us-east-1', 'ami-00c40082600650a9a', 'dummy_client_hash');
INSERT INTO whist.images (provider, region, image_id, client_sha) VALUES ('AWS', 'us-east-2', 'ami-0a7da7479f37c924a', 'dummy_client_hash');
INSERT INTO whist.images (provider, region, image_id, client_sha) VALUES ('AWS', 'us-west-1', 'ami-0bb9ea3cf997fb4ec', 'dummy_client_hash');
INSERT INTO whist.images (provider, region, image_id, client_sha) VALUES ('AWS', 'us-west-2', 'ami-0a2e1892cafee19d4', 'dummy_client_hash');
INSERT INTO whist.images (provider, region, image_id, client_sha) VALUES ('AWS', 'ca-central-1', 'ami-09b2b490cdf85f1f1', 'dummy_client_hash');


--
-- PostgreSQL database dump complete
--
