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


INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, client_commit_hash, enabled) VALUES ('eu-central-1', 'ami-04e791bf07624272e', false, 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, client_commit_hash, enabled) VALUES ('eu-west-1', 'ami-04b85bdd938419795', false, 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, client_commit_hash, enabled) VALUES ('us-east-1', 'ami-00c40082600650a9a', true, 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, client_commit_hash, enabled) VALUES ('us-east-2', 'ami-0a7da7479f37c924a', false, 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, client_commit_hash, enabled) VALUES ('us-west-1', 'ami-0bb9ea3cf997fb4ec', false, 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, client_commit_hash, enabled) VALUES ('us-west-2', 'ami-0a2e1892cafee19d4', false, 'dummy_client_hash', true);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, client_commit_hash, enabled) VALUES ('ca-central-1', 'ami-09b2b490cdf85f1f1', false, 'dummy_client_hash', true);


--
-- Data for Name: supported_app_images; Type: TABLE DATA; Schema: hardware; Owner: uap4ch2emueqo9
--

INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number, task_version) VALUES ('Base', NULL, 'fractal-dev-base', NULL, NULL, NULL, NULL, NULL, true, 0, 238);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number, task_version) VALUES ('Google Chrome', 'https://fractal-supported-app-images.s3.amazonaws.com/chrome-256.svg', 'fractal-dev-browsers-chrome', 'Browser', 'The modern web browser.', 'With Google apps like Gmail, Google Pay, and Google Assistant, Chrome can help you stay productive and get more out of your browser.', 'https://www.google.com/chrome/', 'https://www.google.com/chrome/terms/', true, 0.8, 238);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number, task_version) VALUES ('Firefox', 'https://fractal-supported-app-images.s3.amazonaws.com/firefox-256.svg', 'fractal-dev-browsers-firefox', 'Browser', 'The browser is just the beginning.', 'Firefox is a free web browser that handles your data with respect and is built for privacy anywhere you go online.', 'https://www.mozilla.org/en-US/firefox/', 'https://www.mozilla.org/en-US/about/legal/terms/services/', true, 0, 238);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number, task_version) VALUES ('Blender', 'https://fractal-supported-app-images.s3.amazonaws.com/blender-256.svg', 'fractal-dev-creative-blender', 'Creative', 'The free and open source 3D creation suite.', 'Blender supports the entirety of the 3D pipeline—modeling, rigging, animation, simulation, rendering, compositing and motion tracking, video editing and 2D animation pipeline.', 'https://www.blender.org/', 'https://id.blender.org/terms-of-service', true, 1, 237);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number, task_version) VALUES ('Sidekick', 'https://fractal-supported-app-images.s3.amazonaws.com/sidekick-256.svg', 'fractal-dev-browsers-sidekick', 'Browser', 'The fastest work environment ever made', 'Sidekick is a revolutionary new work OS based on the Chromium browser. Designed to be the ultimate online work experience, it brings together your team and every web tool you use – all in one interface.', 'https://www.meetsidekick.com/', 'https://cdn.meetsidekick.com/Terms+of+Service.pdf', true, 0, 155);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number, task_version) VALUES ('GIMP', 'https://fractal-dev-supported-app-images.s3.amazonaws.com/gimp-256.svg', 'fractal-dev-creative-gimp', 'Creative', 'The free & open source image editor.', 'GIMP is a cross-platform image editor available for GNU/Linux, OS X, Windows and more operating systems. It is free software, so you can change its source code and distribute your changes.', 'https://www.gimp.org/', 'https://www.gimp.org/about/COPYING', true, 0, 155);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number, task_version) VALUES ('Brave', 'https://fractal-supported-app-images.s3.amazonaws.com/brave-256.svg', 'fractal-dev-browsers-brave', 'Browser', 'A fast, private and secure web browser.', 'Brave stops online surveillance, loads content faster, and uses 35% less battery.', 'https://brave.com/', 'https://brave.com/terms-of-use/', true, 0, 155);


--
-- Data for Name: email_templates; Type: TABLE DATA; Schema: sales; Owner: uap4ch2emueqo9
--

INSERT INTO sales.email_templates (id, url, title) VALUES ('EMAIL_VERIFICATION', 'https://fractal-email-templates.s3.amazonaws.com/email_verification.html', '[Fractal] Please verify your email');
INSERT INTO sales.email_templates (id, url, title) VALUES ('PASSWORD_RESET', 'https://fractal-email-templates.s3.amazonaws.com/password_reset.html', 'Reset your password');
INSERT INTO sales.email_templates (id, url, title) VALUES ('PAYMENT_FAILURE', 'https://fractal-email-templates.s3.amazonaws.com/payment_failure.html', 'Action Required: Your payment method failed');
INSERT INTO sales.email_templates (id, url, title) VALUES ('PAYMENT_SUCCESSFUL', 'https://fractal-email-templates.s3.amazonaws.com/payment_successful.html', 'Thank you for choosing Fractal');
INSERT INTO sales.email_templates (id, url, title) VALUES ('FEEDBACK', 'https://fractal-email-templates.s3.amazonaws.com/feedback.html', 'A user has just cancelled their plan');


--
-- PostgreSQL database dump complete
--

