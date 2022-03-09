/* eslint-disable react/no-unescaped-entities */

import React from "react"
import classNames from "classnames"

import { ScreenSize } from "@app/shared/constants/screenSizes"
import { withContext } from "@app/shared/utils/context"

const TermsOfService = () => {
  const { width } = withContext()
  const { dark } = withContext()

  return (
    <div
      className={classNames(
        "overflow-x-hidden",
        dark ? "dark bg-gray-900" : "bg-white"
      )}
      style={{ overflowX: "hidden" }}
      id="top"
    >
      <div
        className="dark:text-gray-300"
        style={{
          padding: width > ScreenSize.MEDIUM ? "75px 150px" : "50px 40px",
          maxWidth: 1280,
          margin: "auto",
        }}
      >
        <div style={{ fontSize: 40 }}>TERMS OF SERVICE</div>
        <div style={{ color: "#555555", marginBottom: 40 }}>
          Last updated March 8th, 2022
        </div>
        <div>
          <p>
            PLEASE READ THESE TERMS OF SERVICE (COLLECTIVELY WITH WHIST'S
            PRIVACY POLICY{" "}
            <a href="https://www.whist.com/privacy">www.whist.com/privacy</a>{" "}
            AND{" "}
            <a href="http://www.copyright.gov/legislation/dmca.pdf">
              www.copyright.gov/legislation/dmca.pdf
            </a>
            , THE "TERMS OF SERVICE ") FULLY AND CAREFULLY BEFORE USING{" "}
            <a href="https://www.whist.com">WWW.whist.com</a> (THE "SITE"), ANY
            DESKTOP OR MOBILE APPLICATIONS PROVIDED BY WHIST (THE
            "APPLICATIONS") AND THE CLOUD COMPUTING SERVICES, FEATURES, CONTENT
            OR APPLICATIONS OFFERED BY WHIST TECHNOLOGIES, INC. ("WHIST," "WE,"
            "US," OR "OUR") (TOGETHER WITH THE SITE AND THE APPLICATIONS, THE
            "SERVICES"). THESE TERMS OF SERVICE SET FORTH THE LEGALLY BINDING
            TERMS AND CONDITIONS FOR YOUR USE OF THE SITE, THE APPLICATIONS AND
            THE SERVICES.
          </p>
          <p>
            YOUR RIGHT TO USE THE SERVICES IS EXPRESSLY CONDITIONED ON
            ACCEPTANCE OF THESE TERMS OF SERVICE. BY CLICKING ON THE "ACCEPT"
            BUTTON AND USING THE SERVICES, YOU UNCONDITIONALLY AGREE TO BE BOUND
            BY THESE TERMS OF SERVICE. IF THESE TERMS OF SERVICE ARE CONSIDERED
            AN OFFER, ACCEPTANCE IS EXPRESSLY LIMITED TO THESE TERMS OF SERVICE.
            IF YOU DO NOT AGREE WITH ANY PROVISION OF THESE TERMS OF SERVICE,
            YOU MUST CLICK ON THE "CANCEL" BUTTON AND MAY NOT ACCESS OR USE THE
            SERVICES IN ANY MANNER FOR ANY PURPOSE.
          </p>
          <p>
            These Terms of Service were last updated on the date specified
            above. It is effective between Whist and you as of the date you
            accept these Terms of Service.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Acceptance Of Terms Of Service
          </p>
          <p>
            By registering for and/or using the Services in any manner,
            including but not limited to visiting or browsing the Site and/or
            downloading the Applications, you agree to these Terms of Service
            and all other operating rules, policies and procedures that may be
            published from time to time on the Site by us, each of which is
            incorporated by reference and each of which may be updated from time
            to time without any notice.
          </p>
          <p>
            Certain of the Services may be subject to additional terms and
            conditions specified by us from time to time; your use of such
            Services is subject to those additional terms and conditions, which
            are incorporated into these Terms of Service by this reference.
          </p>
          <p>
            These Terms of Service apply to all users of the Services,
            including, without limitation, users who are contributors of
            content, information, and other materials or services, registered or
            otherwise.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Eligibility</p>
          <p>
            You represent and warrant that you are at least 18 years of age. If
            you are under age 18, you may not, under any circumstances or for
            any reason, use the Services. We may, in our sole discretion, refuse
            to offer the Services to any person or entity and change its
            eligibility criteria at any time. You are solely responsible for
            ensuring that these Terms of Service are in compliance with all
            laws, rules and regulations applicable to you. The right to access
            the Services is revoked where these Terms of Service or use of the
            Services is prohibited or to the extent offering, sale or provision
            of the Services conflicts with any applicable law, rule or
            regulation. Further, the Services are offered only for your use, and
            not for the use or benefit of any third party.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Registration</p>
          <p>
            To sign up for the Services, you must register for a Whist user
            account (a "User Account") on the Services (an "Account"). You must
            provide accurate and complete information and keep your Account
            information updated. You shall not: (i) use the name of another
            person with the intent to impersonate that person or (ii) use the
            name of a person other than you without appropriate authorization.
            You are solely responsible for the activity that occurs on your
            Account, and for keeping your Account password secure. You may never
            use another person's user account or registration information for
            the Services without permission. You must notify us immediately of
            any change in your eligibility to use the Services (including any
            changes to or revocation of any licenses from state authorities),
            breach of security or unauthorized use of your Account. You should
            never publish, distribute or post login information for your
            Account. You shall have the ability to delete your Account, either
            directly or through a request made to one of our employees or
            affiliates.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Product</p>
          <p>
            Whist is a provider of cloud-based web browsing services.
            Cloud-based web browsing is similar to regulr web browsing in that
            once your Whist subscription is active, the Whist Cloud Browser
            and/or the Whist Cloud Tabs can be interacted with as you normally
            would with a standard web browser, except that some or all of the
            components of the web browser might be hosted on a remote machine
            and delivered to your local device via the Whist's low-latency video
            streaming. Unless indicated otherwise, the remote components of your
            Whist Cloud Browser are hosted on a machine running Linux Ubuntu
            20.04.
          </p>
          <p>
            Whist is committed to our users' privacy and security, which why we
            do not store the data within your cloud browser environment after
            you terminate a Whist Session. A Whist session (“Session”) occurs
            each time you use the Whist Cloud Browser or Whist Cloud Tabs. Each
            Session is standalone, meaning that the remote machine running the
            cloud browsing components of your Whist Cloud Browser or Whist Cloud
            Tabs will be permanently erased shortly after you terminate your
            Session. Whist will, however, store an encrypted version of your web
            browser settings and other states so that we can persist your
            experience across diffeerent Sessions. We do so using a
            locally-stored key on your User Device to ensure that none of our
            team members can access your data. Whist does not assume
            responsibility for any lost user data due to incorrect or forgotten
            saves before terminating a Session.
          </p>
          <p>
            The Whist Cloud Browser and Whist Cloud Tabs can be accessed from
            any Internet-connected device (a “User Device”), provided that we
            release a version of Whist for the device’s operating system.
          </p>
          <p>
            Given the nature of cloud-based personal computing, your inputs
            (keystrokes, mouse/touchpad movements, etc.) are sent from the User
            Device to Whist’s data center infrastructure, where your Whist Cloud
            Browser and Whist Cloud Tabs are hosted. The servers in Whist’s data
            center infrastructure interpret these signals on your Cloud Browser
            and Cloud Tabs, which allows for the Whist Cloud Browserand Whist
            Cloud Tabs to display and function properly. All communications
            between your User Device and the cloud components of your Whist
            Cloud Browser and Cloud Tabs, including input, audio and video
            components of the streaming, are fully AES encrypted, and Whist does
            not observe directly any of your stream components.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Whist Plans And Subscriptions
          </p>
          <p>
            A Whist plan is your subscription to access your Whist Cloud Browser
            and Whist Cloud Tabs. Your subscription period will be calculated
            from the date on which your Whist account is set up and available
            for use (the “Start Date”). You will receive an email informing you
            of that date when you sign up for our service. The subscription will
            continue from the Start Date for the term you selected (the
            “Subscription Period”) and will then automatically renew for
            successive periods equal to your then-current subscription term,
            unless no later than the day preceding the end of the then-current
            term you have cancelled your subscription. The renewed subscription
            will be on the same terms as the current subscription, unless we
            have amended our pricing or other terms before the renewal date,
            which you will be informed of ahead of time should it happen.
          </p>
          <p>
            The cost to you of the Services is the price that was agreed by you
            when you purchased your subscription. This price will remain
            unchanged until the end of your then-current Subscription Period.
            For example, if your subscription period is 30 days, then the cost
            to you will stay the same within the 30 days of that period, payable
            at the beginning of said period. We will inform you reasonably in
            advance of any proposed increase in the price of the Services you
            have purchased. You will have the opportunity to approve or decline
            the renewal of your subscription at the new price, before it is
            charged to your account. However if you have not agreed to the new
            price by the effective date of the proposed price increase, we may
            at our discretion cancel your subscription at the end of the
            then-current Subscription Period, or transition you subscription to
            the plan which is most similar to your then-current subscription.
          </p>
          <p>
            If you selected a monthly plan, or a longer plan that involves
            monthly payments, you will generally be charged a full month at the
            time of purchase of your subscription. If you selected a prepaid
            plan, you will generally be charged the full amount of the plan at
            the time of purchase of your subscription.
          </p>
          <p>
            You are responsible for making on-time payments for your Whist
            subscription, as well as for updating relevant payment information
            in the case of any changes. Whist reserves the right to suspend your
            access to any and all Whist services if you do not make a payment on
            time, and/or charge late payment fees equal to ten (10) US dollars
            per default per week. There may be a delay in return to normal
            Services after default payments have been rendered. If Whist does
            not receive payment for your Whist plan on the day of renewal and
            you have not cancelled your Whist plan, your Services will be
            suspended. If your Services are suspended, Whist is not responsible
            for any refund of previous payments (nor is Whist responsible for
            refunds of previous payments in other situations), and you are still
            responsible for the entire subscription fee outstanding, meaning you
            are still responsible for the entire subscription fee of the current
            subscription period, as defined in your Whist plan. If Services are
            cancelled by Whist because of a failure to pay, the entire remaining
            subscription fee for the subscription period will be payable to
            Whist immediately.
          </p>
          <p>
            You may receive offers to purchase additional optional services from
            Whist or to change your subscription plan. For the most part, those
            changes will be accessible from the User Account. In the case that
            you purchase additional optional services, your purchase of those
            services will become effective immediately, subject to the time
            needed for activation. The optional services or upgrades will be
            billed instantly on a pro rata basis from the date of activation to
            the end of the then-current Subscription Period. Going forward from
            that point, the optional services or upgrades will be added onto the
            main subscription and billed in cycle.
          </p>
          <p>
            If you would like to change subscription plans to a lower-priced
            plan, or you would like to remove optional services, the change
            and/or removal will take effect on the first day of the next
            Subscription Period after the then-current Subscription Period.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Referral Programs</p>
          <p>
            From time to time, Whist may offer its Users benefits for referring
            new additional customers to Whist. Access to any such program is
            subject to compliance with these Terms by both the User and the
            referred User.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Whist Software</p>
          <p>
            Many of the Services offered by Whist allow you, the user, to
            download software in order to use those Services. Software includes,
            but is not limited to, local desktop, tablet, and mobile
            applications that allow the user to use the Whist Cloud Browser
            and/or Whist Cloud Tabs services from User Devices. Whist may
            automatically update Whist software on your device.
          </p>
          <p>
            Whist software is licensed and not sold. If you comply with these
            Terms, Whist grants you a worldwide, limited, personal,
            non-exclusive, non-transferable, non-sub-licensable, and revocable
            license to use the software, only for private and non-commercial
            use, on compatible User Devices belonging to the user, and for the
            sole purpose of enabling the user to access the Services offered by
            Whist. This license is granted to the user for the duration of the
            Subscription Period and will terminate the end of that period. This
            User license includes the right to download and install a copy of
            the software on each User Device.
          </p>
          <p>
            Whist reserves all other rights to the software. These rights
            include, but are not limited to, technological protection and
            privacy measures included in or relating to the software which you,
            the user, are not authorized: to circumvent or avoid; Likewise, you
            are not authorized to: disassemble, decrypt, illegally penetrate,
            reverse-engineer, copy, use, or reconstruct the logic of the
            software; separate the software components to use them on different
            devices; publish, copy, rent, transfer, sell, lend, distribute,
            export, or import the software; make unauthorized use of the
            software in a way that could disrupt services to third parties; or
            attempt to do or assist anyone in doing the aforementioned. Any of
            the above-mentioned behaviors would expose you to prosecution, to
            full extent of the applicable law in your jurisdiction.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Servers And Operating Systems
          </p>
          <p>
            Whist may use any hardware and service providers it sees fit to
            deliver the Whist Services, and those hardware and service providers
            may change over time. It is important to note that the User may be
            using different computing infrastructure in different Sessions
            and/or during a single Session. We will have sole discretion in
            deciding which hardware and service providers we will use in
            connection with our Services, in compliance with local regulations
            where the hardware is hosted; the User acknowledges and agrees that
            they may not make any complaints or demands regarding Whist’s choice
            in hardware and service providers.
          </p>
          <p>
            The Whist cloud browsing components are collectively hosted on
            Microsoft Azure, Amazon Web Services and Google Cloud Platform,
            depending on the world region you are accessing them from, the time
            of day, the current demand for Whist, etc. Whist abides by the Terms
            of Service of the respective cloud providers for which it uses the
            cloud infrastructure to service Whist Cloud Browsers and Whist Cloud
            Tabs, and your agreement in the Whist Terms of Service extends to
            the Terms of Service of the cloud providers from which your Whist
            experience is being serviced. We encourage you to review the Terms
            of Service of the three cloud providers from which Whist services
            cloud-based applications. Other cloud providers may be added from
            time-to-time to enable Whist to service more users. If this is the
            case, you will be notified ahead of time. By making continuous usage
            of the Whist Services following the addition of a new cloud provider
            by Whist, you express your agreement of their Terms of Service.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Internet Access</p>
          <p>
            Use of Whist’s Services requires a working Internet connection,
            which is not included in our Services. The quality of Whist’s
            Services will depend on the quality of the User’s Internet
            connection, in terms of bandwidth, stability, ping, jitter, and a
            variety of other factors dependent on the properties of the Internet
            connection. The User is responsible for their own Internet
            connection. Whist cannot be held responsible for interruptions or
            degradations in our Services due to a User’s Internet connection; we
            will not offer any refund if access to our Services has been
            interrupted or degraded by your Internet connection.
          </p>
          <p>
            You acknowledge that our Services may require the transfer of large
            amounts of data over your Internet connection and accept that Whist
            is not responsible for any costs payable to your Internet service
            provider.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Code Of Conduct</p>
          <p>
            The proper functioning of our Services requires that Users use the
            Services in an appropriate and reasonable way. Most notably, the
            Whist Cloud Browser and Whist Cloud Tabs services may only be used
            as a personal application, for the private use of the Whist User,
            whether in the context of personal usage or that of personal
            business usage. This excludes any business or professional purpose
            which exceeds a personal business computing, like running a server,
            mining cryptocurrencies, selling access to your Whist subscription,
            etc.
          </p>
          <p>
            Without limiting the above, you will not use Whist’s Services for
            any of the following:
          </p>
          <ul>
            <li>
              Advocating or perpetrating illegal activities of any kind,
              including any activities which would violate local, state,
              national, federal, or international laws, rules, and regulations;
            </li>
            <li>
              Creating, uploading, linking or sharing fraudulent, defamatory or
              misleading content, or intending to incite crimes and offences,
              racial hate or suicide, justify crimes against humanity, or
              containing child pornography, or any other content of a violent or
              pornographic nature where the content could be accessed by minors;
            </li>
            <li>
              Infringing any third party's copyright, patent, trademark, trade
              secret or other proprietary rights or rights of publicity or
              privacy, or using the Services to share copyrighted material that
              you do not own or have permission to share or distribute;
            </li>
            <li>
              Disseminating any harassing, slanderous, defamatory, sexually
              explicit, libelous, racist, indecent, abusive, violent,
              threatening, intimidating, harmful, vulgar, obscene, offensive or
              otherwise objectionable material of any kind or nature, or
              infringing the personal privacy or rights of third parties;
            </li>
            <li>
              Hacking into third-party computer systems, hosting botnet-type
              aggressive services, spreading viruses, worms, spyware, time bombs
              or other computer programs with the purpose or effect of
              restricting, harming or altering the proper functioning of
              hardware or computer programs;
            </li>
            <li>
              Posting, distributing, or otherwise making available or
              transmitting any software or other computer file that contains a
              virus, trojan horse, worm, malware or other harmful or destructive
              component;
            </li>
            <li>
              Mining cryptocurrencies, any related activities, or using Whist’s
              computing power to break encryption keys;
            </li>
            <li>
              Sending unwanted messages, promotions or advertising, or spam, or
              modified, misleading or false source identification information,
              including by spoofing or phishing techniques, and in general,
              taking the identity of any other person whatsoever;
            </li>
            <li>
              Reselling the Services or otherwise making the Services available
              to a third party, including using Whist Cloud Applications as a
              server or with Software that functions as a server, or using Whist
              for commercial or business purposes;
            </li>
            <li>
              Hampering or attempting to hamper, in any way whatsoever, the
              proper functioning of the Services, including disabling, altering,
              infringing or circumventing, or attempting to disable, alter, or
              circumvent, in any form whatsoever, any device or feature of the
              Services, in particular any security or authentication feature,
              access restriction, storage limit, or any standby or shut-down
              mechanism, or modifying or using non-public areas of the Services
              or common areas of the Services which you are asked not to access;
            </li>
            <li>
              Deleting from the Service or the Software any legal notices,
              disclaimers, or proprietary notices such as copyright or trademark
              notices, or using or modifying any logo or other content of the
              Websites without Whist’s prior written consent;
            </li>
            <li>
              Printing, copying or reverse engineering any code or Whist
              hardware, including any Software;
            </li>
            <li>
              Taking any action that degrades, harms, disrupts or interferes
              with the Services, the cloud equipment that operates Whist, or the
              security of the Services or, more generally, taking any action
              that could be harmful to Whist or other users;
            </li>
            <li>
              Analyzing, probing, or testing the vulnerability of any system or
              network.
            </li>
          </ul>
          <p>
            We reserve the right to review your conduct and content for
            compliance with these Terms, and to suspend your access to the
            Services and/or cancel your subscription, or take such other action
            as we may in our discretion deem appropriate, in the event that we
            have reason to believe that you have violated these Terms.
          </p>
          <p>
            We also reserve the right to monitor, amend and/or remove any
            content posted on the Websites. Whist will not assume any liability
            arising from the content in your (the user’s) account. You are
            responsible for the content on your account.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Non-Whist Provided Services And Applications
          </p>
          <p>
            The Services may include software, applications, websites and
            services which are offered, controlled or operated by third-parties
            unaffiliated with Whist (the “Third-Party Applications”). We may
            also allow you to purchase Third-Party Applications from our
            Services. These Third-Party Applications may have their own Terms of
            Service, in which case you may be required to accept them in order
            to access the relevant Third-Party Applications. We are not
            responsible for, and disclaim all liability which may arise out of
            or in connection with, any Third-Party Applications that you use or
            purchase through the Whist Services. We encourage you to review
            these Terms of Service frequently.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            User’s Data And Content
          </p>
          <p>
            While using the Whist Services, you may send us various files,
            content, emails, contacts, etc. (the “User Content”). The User
            Content belongs to the User and in no way to Whist. Whist is only
            granted the right to use the User Content to the extent required for
            the proper functioning of the Services. These limited rights include
            in particular, the right to temporarily host, save, and share the
            User Content, for the intended functioning of the User’s Whist Cloud
            Browser and Whist Cloud Tabs. User Content is not stored by Whist,
            with the exception of third-party authentication integrations,
            plugins and preferences you specifiy to us to improve your
            experience, and gets permanently deleted shortly after the end of a
            Whist Session. It is the User's responsibility to save the User
            Content.
          </p>
          <p>
            You acknowledge and agree that Whist may remove User Content from
            the Services if you are in violation of these Terms or if we cancel
            or suspend our Services; we are not liable for deletion of User
            Content or accidental loss User Content. We strongly advise that
            Users backup User Content in many different places. Some of our
            Services may allow you to share your User Content with third
            parties; Whist is not liable for the sharing of any User Content
            with third parties.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Personal Data</p>
          <p>
            In addition to the User Content temporarily saved on the User's
            Cloud Browser and Cloud Tabs hardware during a Whist Session, Whist
            collects certain personal data concerning the User. For example,
            Whist will collect information such as your name, email address,
            phone number, payment information, physical address, date of birth,
            and data concerning your purchase of Services. In addition, Whist
            will collect information related to how and when you use the
            Services, including any User Devices used to access the Services,
            solely with a view to performing or improving its Services,
            including adjusting the video streaming to the User's Internet
            connection and allocating its hardware resources based on its Users’
            needs.
          </p>
          <p>
            You hereby authorize Whist to store, process and use the foregoing
            data, and communicate such data to our affiliates, in each case for
            the purposes set forth above.
          </p>
          <p>
            For the efficient and proper running of the Websites, Whist may use
            cookies, which are files used to identify the User each time they
            connect. Cookies are used to improve the personalized services
            offered and for statistical purposes. The User may object to the use
            of cookies, by changing his or her browser settings, without
            relinquishing access to the service.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Websites User License
          </p>
          <p>
            Whist grants the User, subject to compliance with these Terms, a
            limited, non-exclusive, non-transferable, non-sub-licensable and
            revocable license for non-commercial, personal, private access,
            browsing and use of the Websites.
          </p>
          <p>
            Whist grants the User a non-exclusive and revocable right to create
            hyperlinks to the home page of the Websites, provided that they do
            not portray Whist or its Services in a misleading, derogatory or
            offensive way, or more generally, provided that they do not affect
            Whist in any way whatsoever.
          </p>
          <p>
            The reproduction of any documents published on the Websites is only
            permitted for information purposes, and for personal and private use
            only. Any commercial use of the documents is strictly prohibited.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Intellectual Property
          </p>
          <p>
            All intellectual property associated with the Services and the
            Software (jointly referred to as the "Whist IP") are proprietary to
            Whist and/or its affiliates and/or its or their suppliers, and are
            protected by copyrights, trademarks, service marks, patents and/or
            other proprietary rights and laws. Whist IP includes, without
            limitation, any trademarks, logos, trade names, photographs,
            publications, texts, documents, descriptions, slogans, domain names,
            patents, know-how, software, source code, applications, user
            interfaces, databases, drawings, designs and models, designs, works,
            images, graphs, illustrations, digital downloads, animated and audio
            sequences, and all other intellectual works associated with the
            Services.
          </p>
          <p>
            You acknowledge that by purchasing the Services you are not
            acquiring any right in or title to the Whist IP. Except as
            specifically permitted herein, no portion of the Whist IP may be
            used or reproduced in any form, or by any means. All intellectual
            property rights not expressly granted in these Terms are reserved to
            Whist and its affiliates.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Termination By Us</p>
          <p>
            We expect you to comply with these Terms. Whist reserves the right
            to suspend the Services, without notice, if you breach these Terms
            (including, in particular, in the event of a payment default or a
            violation of our Code of Conduct, which can be found above in these
            Terms). If a breach has not been cured within seven (7) days from an
            email notice from us, we may elect to terminate your subscription,
            in which case you will receive an email and you will become
            immediately liable for the price of all the Services included in
            your then-current Subscription Period. If you have selected a
            monthly plan, your plan will be automatically suspended immediately
            without refund for the current billing period and your Whist Account
            data will be kept for internal reference. If you have selected a
            longer prepaid plan, your plan will be automatically suspended
            immediately without refund for the entire period of your prepaid
            plan and your Whist Account data will be kept for internal
            reference.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Termination By You</p>
          <p>
            You may decide to end your subscription at any time, by reaching out
            to your support team via email, or by any other means that may be
            indicated on the Website. If you decide to terminate your
            subscription, the Services and the corresponding payments will
            continue until the end of the then-current Subscription Period, and
            will stop at the end of the period; we will not offer any refund.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Territories</p>
          <p>
            Due to the nature of cloud-based web browsing, the Whist Services
            may not be officially supported in certain regions and countries.
            Whist will only add official support for a specific geography once
            it has cloud computing infrastructure available in that region, to
            guarantee a high quality experience for our users. Because of the
            nature of the Internet, Whist might be accessible in the country or
            region in which you are physically present at a specific moment,
            even if this country or region is not part of the officially
            supported regions. Furthermore, access from those other geographies
            may be possible, but the distance between the User and the Whist
            data center infrastructure may cause diminished material quality of
            Services. Whist only guarantees quality of service in officially
            supported regions; usage of the Whist Services outside of those
            territories is not endorsed by Whist and the User is reponsible to
            respect the local laws, privacy regulations and other restrictions
            which may be imposed upon them by local authorities. Whist declines
            all liabilities for usage of our Services outside of
            officially-supported geographies. If you are not sure whether Whist
            officially support a certain geography, you can contact us by email
            at <a href="mailto:support@whist.com">support@whist.com</a>.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Copyright</p>
          <p>
            We respect the intellectual property of others and ask our Users to
            do the same.
          </p>
          <p>
            If you believe that any content made available on or through our
            Services has been used or exploited in a manner that infringes an
            intellectual property right you own or control, then please promptly
            send a DMCA Notice to the Designated Agent identified below:
          </p>
          <p>
            Whist Technologies, Inc. <br />
            33 Irving Pl <br />
            New York, NY 10003 <br />
            <a href="mailto:support@whist.com">support@whist.com</a> <br />
          </p>
          <p>
            We reserve the right to delete or disable any content alleged to be
            infringing, and/or terminate the subscription of repeat infringers.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Disclaimer; Liability; Indemnification
          </p>
          <p>
            YOU ACKNOWLEDGE AND AGREE THAT THE SERVICES ARE PROVIDED ON AN "AS
            IS" AND “AS AVAILABLE” BASIS.
          </p>
          <p>
            TO THE FULLEST EXTENT PERMITTED BY LAW, (A) WHIST MAKES NO
            WARRANTIES, EXPRESS OR IMPLIED, WITH RESPECT TO THE SERVICES; (B)
            WHIST DISCLAIMS ANY REPRESENTATIONS AND WARRANTIES, INCLUDING
            WITHOUT LIMITATION AS TO MERCHANTABILITY, FITNESS FOR A PARTICULAR
            PURPOSE AND NON-INFRINGEMENT; AND (C) EXCEPT FOR ANY LIABILITY FOR
            FRAUD, FRAUDULENT MISREPRESENTATION OR GROSS NEGLIGENCE, IN NO EVENT
            SHALL WHIST OR ITS AFFILIATES BE LIABLE TO YOU OR ANY THIRD-PARTY
            FOR (1) ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL, EXEMPLARY OR
            CONSEQUENTIAL DAMAGES, OR (2) ANY LOSS OF USE, DATA, BUSINESS,
            GOODWILL, OR PROFITS, OR THE LOSS OF USER CONTENT, OR (3) ANY DAMAGE
            TO USER’S HARDWARE OR SOFTWARE, EVEN, IN EACH CASE, IN CIRCUMSTANCES
            WHERE WHIST WAS WARNED OF THE POSSIBILITY OF SUCH DAMAGES.
          </p>
          <p>
            IN ADDITION, OTHER THAN FOR THE TYPES OF LIABILITY WE CANNOT LIMIT
            BY LAW, WHIST AND ITS AFFILIATES’ LIABILITY FOR ALL CLAIMS RELATING
            TO ANY SERVICE WILL BE CAPPED AT THE HIGHER OF $100 OR THE AMOUNTS
            PAID BY YOU TO WHIST FOR THE SERVICE CONCERNED, DURING THE SIX
            MONTHS PRECEDING THE EVENT GIVING RISE TO THE CLAIM. FINALLY, YOU
            WILL DEFEND, INDEMNIFY AND HOLD WHIST, ITS AFFILIATES, AND ITS AND
            THEIR DIRECTORS, OFFICERS AND EMPLOYEES HARMLESS FROM ANY CLAIM,
            COST, LIABILITY, LOSS OR SETTLEMENT INCURRED IN CONNECTION WITH A
            THIRD-PARTY CLAIM ARISING OUT OF OR IN CONNECTION WITH A VIOLATION
            BY YOU OF ANY OF THESE TERMS.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Amendment Of The Conditions
          </p>
          <p>
            Whist may from time to time amend these Terms. In that case, you
            will be asked to accept the amended Terms that will apply as of the
            start of your next billing period. If you do not accept the amended
            Terms, the Services will continue in accordance with these Terms,
            until the end of your then-current Subscription Period. If by that
            date you have not agreed to the amended Terms, we reserve the right
            to immediately cancel your subscription.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Notices</p>
          <p>
            Unless otherwise stated in these Terms, when the User seeks to
            contact Whist, the user should do so through Whist’s website.
          </p>
          <p>
            Whist may contact the User by any useful means, including by email
            or text message sent to the contact details provided by the User in
            their User Account or via the Websites.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>Miscellaneous</p>
          <p>
            These Terms constitute the entire agreement between you and Whist
            with respect to the subject matter of these Terms, and supersede and
            replace any other prior or contemporaneous agreements, or terms and
            conditions applicable to the subject matter of these Terms. These
            Terms create no third-party beneficiary rights.
          </p>
          <p>
            Whist’s failure to enforce a provision of these Terms shall not be
            deemed a waiver of its right to do so in the future. If a provision
            of these Terms is found to be unenforceable, the remaining
            provisions of these Terms will remain in full effect and an
            enforceable term will be substituted reflecting as closely as
            possible the intent of the parties. You may not assign any of your
            rights under these Terms, and any such attempt will be void. Whist
            may assign its rights to any of its affiliates, or to any successor
            in interest of any business associated with the Services.
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Applicable Law; Disputes
          </p>
          <p>
            These Terms will be governed by New York, U.S.A. law except for its
            conflicts of laws principles, unless otherwise required by a
            mandatory law of any other jurisdiction.
          </p>
          <p>
            The parties shall endeavor to settle any disputes regarding these
            Terms or the Services amicably before submitting the case to the
            competent courts.
          </p>
          <p>
            To that effect, before filing a claim against us, you agree to try
            to resolve the dispute informally by opening a support ticket via
            email to <a href="mailto:support@whist.com">support@whist.com</a>.
            We will try to resolve the dispute informally. If a dispute is not
            resolved within fifteen days of submission, you or we may bring a
            formal proceeding.
          </p>
          <p>
            Any judicial proceeding to resolve claims relating to these Terms or
            the Services shall be brought in the federal or state courts of New
            York, New York, subject to the mandatory arbitration provisions
            below. By accepting these terms, you consent to our choice of venue
            and submit to the jurisdiction of the federal and/or state courts of
            New York, New York (or other courts, if we so choose).
          </p>
          <p style={{ fontWeight: "bold", fontSize: 20 }}>
            Mandatory Arbitration
          </p>
          <p>
            You and we agree to resolve any claims relating to these Terms or
            the Services through final and binding arbitration by the American
            Arbitration Association (AAA), in accordance with its Commercial
            Arbitration Rules and the Supplementary Procedures for Consumer
            Related Disputes. The AAA rules will govern payment of all
            arbitration fees. The arbitration will be held in the United States
            county where you live or work or in New York, New York, or any other
            location we agree upon. If the agreement to arbitrate is found not
            to apply to you or your claim, you agree to the exclusive
            jurisdiction of the state and federal courts in New York, New York
            to resolve your claim.
          </p>
          <p>
            You can opt-out of the requirement to arbitrate by emailing{" "}
            <a href="mailto:support@whist.com">support@whist.com</a> with the
            subject line “Arbitration Opt-Out” within 30 days of the date you
            first register your account. You can obtain the opt-form, free of
            charge, by contacting Whist via the email address provided. This
            step is not necessary if you have already opted-out in a previous
            version of these terms: your previous decision regarding arbitration
            is still binding unless you update it.
          </p>
          <p>
            Notwithstanding the foregoing, either you or we may assert claims in
            small claims court in New York (NY) or any United States county
            where you live or work. Either party may bring a lawsuit solely for
            injunctive relief to stop unauthorized use or abuse of the Services,
            violation of these Terms, or intellectual property infringement
            without first engaging in arbitration or the informal
            dispute-resolution process described above.
          </p>
          <p>
            Class arbitrations, class actions, private attorney general actions,
            and consolidation with other arbitrations are NOT allowed. You may
            not bring a claim as a plaintiff or a class member in a class,
            consolidated, or representative action. You may only resolve
            disputes with us on an individual basis. If this specific paragraph
            is held unenforceable, then the entirety of this "Mandatory
            Arbitration Provisions" section will be deemed void.
          </p>
        </div>
      </div>
    </div>
  )
}

export default TermsOfService
