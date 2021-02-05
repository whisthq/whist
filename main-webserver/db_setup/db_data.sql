--
-- PostgreSQL database dump
--

-- Dumped from database version 12.5 (Ubuntu 12.5-1.pgdg16.04+1)
-- Dumped by pg_dump version 13.1

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

INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, region_being_updated) VALUES ('us-east-1-original', 'ami-0656db14219ddffd0', false, false);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, region_being_updated) VALUES ('eu-central-1', 'ami-0399500a97a69c2a7', true, false);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, region_being_updated) VALUES ('eu-west-1', 'ami-014b671071e6ba3ba', true, false);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, region_being_updated) VALUES ('us-east-1', 'ami-084d662c092c3ca03', true, false);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, region_being_updated) VALUES ('us-east-2', 'ami-095526bd87bd5b79b', false, false);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, region_being_updated) VALUES ('us-west-1', 'ami-0b8799887e9a1dc91', true, false);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, region_being_updated) VALUES ('us-west-2', 'ami-0fb76af699ccc5a88', true, false);
INSERT INTO hardware.region_to_ami (region_name, ami_id, allowed, region_being_updated) VALUES ('ca-central-1', 'ami-01f629b847225fad4', true, false);


--
-- Data for Name: supported_app_images; Type: TABLE DATA; Schema: hardware; Owner: uap4ch2emueqo9
--

INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Slack', 'https://fractal-supported-app-images.s3.amazonaws.com/slack-256.svg', 'fractal-productivity-slack', 'Productivity', 'One platform for your team and your work.', 'Slack allows you to bring your team together with channels, share files, connect on voice and video calls, and connect with external partners. It is a fast, organized, and secure application to communicate in your workplace.', 'https://slack.com/', 'https://slack.com/terms-of-service/user', true, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Figma', 'https://fractal-supported-app-images.s3.amazonaws.com/figma-256.svg', 'fractal-creative-figma', 'Design', 'Where teams design together.', 'Figma helps teams create, test, and ship better designs from start to finish.', 'https://www.figma.com/', 'https://www.figma.com/tos/', true, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Base', NULL, 'fractal-base', NULL, NULL, NULL, NULL, NULL, false, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('GIMP', 'https://fractal-supported-app-images.s3.amazonaws.com/gimp-256.svg', 'fractal-creative-gimp', 'Creative', 'The free & open source image editor.', 'GIMP is a cross-platform image editor available for GNU/Linux, OS X, Windows and more operating systems. It is free software, so you can change its source code and distribute your changes.', 'https://www.gimp.org/', 'https://www.gimp.org/about/COPYING', true, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Sidekick', 'https://fractal-supported-app-images.s3.amazonaws.com/sidekick-256.svg', 'fractal-browsers-sidekick', 'Browser', 'The fastest work environment ever made', 'Sidekick is a revolutionary new work OS based on the Chromium browser. Designed to be the ultimate online work experience, it brings together your team and every web tool you use – all in one interface.', 'https://www.meetsidekick.com/', 'https://cdn.meetsidekick.com/Terms+of+Service.pdf', true, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Lightworks', 'https://fractal-supported-app-images.s3.amazonaws.com/lightworks-256.svg', 'fractal-creative-lightworks', 'Creative', 'The complete video creation package.', 'Lightworks is the complete video creative package so everyone can make video that stands out from the crowd. Whether you need to make video for social media, YouTube or for a 4K film project, Lightworks makes it all possible!', 'https://www.lwks.com/', 'https://www.lwks.com/index.php?option=com_content&view=article&id=26', false, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('TextureLab', 'https://fractal-supported-app-images.s3.amazonaws.com/texturelab-256.svg', 'fractal-creative-texturelab', 'Creative', 'Textures made easy.', 'Build textures fast. Customize textures to your liking before downloading them.', 'https://texturelab.io/', 'https://texturelab.io/account/register', true, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Discord', 'https://fractal-supported-app-images.s3.amazonaws.com/discord-256.svg', 'fractal-productivity-discord', 'Productivity', 'Your place to talk.', 'Whether you’re part of a school club, gaming group, worldwide art community, or just a handful of friends that want to spend time together, Discord makes it easy to talk every day and hang out more often.', 'https://discord.com/', 'https://discord.com/terms', true, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Blockbench', 'https://fractal-supported-app-images.s3.amazonaws.com/blockbench-256.svg', 'fractal-creative-blockbench', 'Creative', 'A modern 3D model editor for cube-based models.', 'Blockbench is a free, modern model editor for boxy models and pixel art textures. Models can be exported for Minecraft Java and Bedrock Edition as well as most game engines and other 3D applications.  Blockbench features a modern and intuitive UI, plugin support and innovative features. It is the industry standard for creating custom 3D models for the Minecraft Marketplace.', 'https://blockbench.net/', 'https://blockbench.net/faq/', false, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Brave', 'https://fractal-supported-app-images.s3.amazonaws.com/brave-256.svg', 'fractal-browsers-brave', 'Browser', 'A fast, private and secure web browser.', 'Brave stops online surveillance, loads content faster, and uses 35% less battery.', 'https://brave.com/', 'https://brave.com/terms-of-use/', true, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Firefox', 'https://fractal-supported-app-images.s3.amazonaws.com/firefox-256.svg', 'fractal-browsers-firefox', 'Browser', 'The browser is just the beginning.', 'Firefox is a free web browser that handles your data with respect and is built for privacy anywhere you go online.', 'https://www.mozilla.org/en-US/firefox/', 'https://www.mozilla.org/en-US/about/legal/terms/services/', true, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Blender', 'https://fractal-supported-app-images.s3.amazonaws.com/blender-256.svg', 'fractal-creative-blender', 'Creative', 'The free and open source 3D creation suite.', 'Blender supports the entirety of the 3D pipeline—modeling, rigging, animation, simulation, rendering, compositing and motion tracking, video editing and 2D animation pipeline.', 'https://www.blender.org/', 'https://id.blender.org/terms-of-service', true, 1);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Suriya Chrome', 'https://fractal-supported-app-images.s3.amazonaws.com/chrome-256.svg', 'fractal-browsers-suriya-chrome', 'Browser', 'The modern web browser.', 'With Google apps like Gmail, Google Pay, and Google Assistant, Chrome can help you stay productive and get more out of your browser.', 'https://www.google.com/chrome/', 'https://www.google.com/chrome/terms/', false, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Cloud Storage Test', 'https://fractal-supported-app-images.s3.amazonaws.com/chrome-256.svg', 'cloud-storage-test', 'Browser', 'The modern web browser.', 'With Google apps like Gmail, Google Pay, and Google Assistant, Chrome can help you stay productive and get more out of your browser.', 'https://www.google.com/chrome/', 'https://www.google.com/chrome/terms/', false, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Google Chrome', 'https://fractal-supported-app-images.s3.amazonaws.com/chrome-256.svg', 'fractal-browsers-chrome', 'Browser', 'The modern web browser.', 'With Google apps like Gmail, Google Pay, and Google Assistant, Chrome can help you stay productive and get more out of your browser.', 'https://www.google.com/chrome/', 'https://www.google.com/chrome/terms/', true, 1);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('Notion', 'https://fractal-supported-app-images.s3.amazonaws.com/notion-256.svg', 'fractal-productivity-notion', 'Productivity', 'The all-in-one workspace.', 'Notes, tasks, wikis, & databases. One tool for your whole team - write, plan, and get organized.', 'https://www.notion.so/', 'https://www.notion.so/Terms-Conditions-4e1c5dd3e3de45dfa4a8ed60f1a43da0', true, 0);
INSERT INTO hardware.supported_app_images (app_id, logo_url, task_definition, category, description, long_description, url, tos, active, preboot_number) VALUES ('SavvyECSTest', NULL, 'savvy-test-ecs-taskdef-1', NULL, NULL, NULL, NULL, NULL, true, 0);


--
-- PostgreSQL database dump complete
--

