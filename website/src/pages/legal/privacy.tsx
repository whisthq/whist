/* eslint-disable react/no-unescaped-entities */

import React from "react"
import classNames from "classnames"

import { ScreenSize } from "@app/shared/constants/screenSizes"
import { withContext } from "@app/shared/utils/context"

const Privacy = () => {
    const { width } = withContext()
    const { dark } = withContext()
    return (
        <div
            className={classNames(
                "overflow-x-hidden",
                dark ? "dark bg-blue-darkest" : "bg-white"
            )}
            style={{ overflowX: "hidden" }}
            id="top"
        >
            <div
                className="dark:text-gray-300"
                style={{
                    padding:
                        width > ScreenSize.MEDIUM ? "75px 150px" : "50px 40px",
                    maxWidth: 1280,
                    margin: "auto",
                }}
            >
                <div style={{ fontSize: 40 }}>PRIVACY POLICY</div>
                <div style={{ color: "#555555", marginBottom: 40 }}>
                    Last updated July 16th, 2021
                </div>

        <p>
          Thank you for choosing to be part of our community at Fractal
          Computers, Inc., doing business as Fractal (“Fractal”, “we”, “us”, or
          “our”). When you visit our website,{" "}
          <a href="https://www.fractal.co">www.fractal.co</a>, and use our
          services, you trust us with your personal information. We want you to
          know that we are committed to protecting your personal information and
          your right to privacy. This privacy policy explains what happens to
          the personal information we collect through our website (such as{" "}
          <a href="https://www.fractal.co">www.fractal.co</a>
          ), and/or any related services, sales, marketing or events (we refer
          to them collectively in this privacy policy as the "Services"). This
          policy will also identify your rights regarding your personal
          information. If you have any questions or concerns about our policy,
          or our practices with regards to your personal information, please
          contact us at{" "}
          <a href="mailto:support@fractal.co">support@fractal.co</a>. We
          encourage you to read through this important information. If there are
          any terms in this privacy policy that you disagree with, please
          discontinue the use of our Sites and our Services.
        </p>

        <p style={{ fontWeight: "bold", fontSize: 20 }}>Table Of Contents</p>
        <ol>
          <li>What Information Do We Collect?</li>
          <li>How Do We Use Your Information?</li>
          <li>Will Your Information Be Shared With Anyone?</li>
          <li>Do We Use Cookies And Other Tracking Technologies?</li>
          <li>How Do We Handle Your Social Logins?</li>
          <li>How Long Do We Keep Your Information?</li>
          <li>How Do We Keep Your Information Safe?</li>
          <li>Do We Collect Information From Minors?</li>
          <li>Advertising And Analytics Services Provided By Others</li>
          <li>What Are Your Privacy Rights?</li>
          <li>Controls For Do-not-track Features</li>
          <li>Do California Residents Have Specific Privacy Rights?</li>
          <li>Do We Make Updates To This Policy?</li>
          <li>How Can You Contact Us About This Policy?</li>
          <li>
            How Can You Review, Update, Or Delete The Data We Collect From You?
          </li>
        </ol>

                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    1. What Information Do We Collect?
                </p>
                <p>
                    Much of the information we collect is personal information
                    you disclose to us. For example, this includes all publicly
                    available personal information, information provided by you
                    through your usage, credentials and payment data. Likewise,
                    we use online identifiers to collect information
                    automatically.
                </p>
                <p style={{ fontWeight: "bold" }}>
                    Personal Information You Disclose To Us
                </p>
                <p>
                    <u>Summary:</u> We collect personal information that you
                    provide to us such as your name, address, contact
                    information, passwords and security data, payment
                    information, and social media login data.
                </p>
                <p>
                    We also collect personal information that you provide when
                    you register your interest in our services and products,
                    when you register your interest in information and
                    promotional offers regarding our services, when you register
                    to participate in activities related to the service, or when
                    you contact us.
                </p>
                <p>
                    The personal information that we collect depends on the type
                    of interaction you have with our Services, the choices you
                    make and the products and features you use. We collect
                    personal information, including but not limited to: publicly
                    available personal information, personal information
                    provided by you, credentials, and payment details.
                </p>
                <div style={{ paddingLeft: 20 }}>
                    <p style={{ fontWeight: "bold" }}>
                        Publicly Available Personal Information
                    </p>
                    <p>
                        We collect first name, maiden name, last name, and
                        pseudonyms email addresses; business email; business
                        phone number; phone numbers; social media; state of
                        residence; country of residence and other similar data.
                    </p>
                    <p style={{ fontWeight: "bold" }}>
                        Personal Information Provided By You
                    </p>
                    <p>
                        We collect application usage data; application usage
                        location; data collected from surveys; and other similar
                        data.
                    </p>
                    <p style={{ fontWeight: "bold" }}>Credentials</p>
                    <p>
                        We may collect passwords, password hints, and similar
                        security information used for authentication and account
                        access.
                    </p>
                    <p style={{ fontWeight: "bold" }}>Payment Data</p>
                    <p>
                        We collect data necessary to process your payment if you
                        make purchases, such as your payment instrument
                        reference numbers. For example, if you are using a
                        credit card we would collect the credit card’s number,
                        its card verification code and its expiration date. All
                        payment data is stored by Stripe. You may find their
                        privacy policy linked here:{" "}
                        <a href="https://www.stripe.com/privacy">
                            stripe.com/privacy
                        </a>
                        .
                    </p>
                    <p style={{ fontWeight: "bold" }}>
                        Social Media Login Data
                    </p>
                    <p>
                        We may provide you with the option to register using
                        social media account details, like your Apple, Facebook,
                        Google, Twitter or other social media account. If you
                        register in this way, we will collect the Information
                        described in the section called "HOW DO WE HANDLE YOUR
                        SOCIAL LOGINS" below.
                    </p>
                    <p>
                        All personal information that you provide to us must be
                        true, complete and accurate, and you must notify us of
                        any changes to such personal information. Failure to do
                        so might result in your access to our Services being
                        jeopardized.
                    </p>
                </div>
                <p style={{ fontWeight: "bold" }}>
                    Information Automatically Collected
                </p>
                <p>
                    <u>Summary:</u> Some information — such as IP address and/or
                    browser and device characteristics — is collected
                    automatically when you visit our Services but it is not tied
                    to your name.
                </p>
                <p>
                    Each time you interact with the Services, we automatically
                    collect some information to maintain the security and
                    operation of our Services, generate internal analytics and
                    comply with reporting requirements. The information
                    collected for these purposes may include your device and
                    usage information, IP address, browser and device
                    characteristics, operating system, hardware specifications,
                    language preferences, referring URLs, device name, country,
                    location, information about how and when you use our
                    Services and other technical information. However, this
                    information is not connected to your name or other personal
                    identifiers (like your contact details).
                </p>
                <div style={{ paddingLeft: 20 }}>
                    <p style={{ fontWeight: "bold" }}>Online Identifiers</p>
                    <p>
                        We collect device identification (i.e. type, make, and
                        other identifiers); applications; tools and protocols,
                        such as IP (Internet Protocol) addresses; cookie
                        identifiers, or others such as the ones used for
                        analytics and marketing; device's geolocation; and other
                        similar data.
                    </p>
                </div>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    2. How Do We Use Your Information?
                </p>
                <p>
                    <u>Summary:</u> We process your information with your
                    consent to advance our legitimate business interests in
                    compliance with our legal obligations. We use personal
                    information collected via our Services for a variety of
                    business purposes described below. We process your personal
                    information for these purposes in reliance on our legitimate
                    business interests, in order to enter into or perform a
                    contract with you, with your consent, and/or for compliance
                    with our legal obligations. We indicate the specific
                    processing grounds we rely on next to each purpose listed
                    below.
                </p>
                <div style={{ paddingLeft: 20 }}>
                    <p style={{ fontWeight: "bold" }}>
                        We Use The Information We Collect Or Receive:
                    </p>
                    <p>
                        To facilitate account creation and logon process. If you
                        choose to link your account with us to a third party
                        account (such as your Google or Facebook account), we
                        use the information you allowed us to collect from those
                        third parties to facilitate account creation and logon
                        process for the performance of the contract. See the
                        section below headed "How Do We Handle Your Social
                        Logins" for further information.
                    </p>
                    <p>
                        To send you personalized marketing and promotional
                        communications. We and/or our third party marketing
                        partners may use the personal information you send to us
                        for our marketing purposes, if this is in accordance
                        with your marketing preferences. You can opt-out of our
                        marketing emails at any time (see the "What Are Your
                        Privacy Rights" section below).
                    </p>
                    <p>
                        To send administrative information to you. We may use
                        your personal information to send you information about
                        our product, service and new features, or information
                        about changes to our terms, conditions, and policies.
                    </p>
                    <p>
                        To fulfill and manage your orders. We may use your
                        information to fulfill and manage your orders, payments,
                        returns, and exchanges made through the Services.
                    </p>
                    <p>
                        To post testimonials. We post testimonials on our
                        Services that may contain personal information. Prior to
                        posting a testimonial, we will seek your consent to use
                        your name and testimonial. If you wish to update or
                        delete your testimonial, please contact us at{" "}
                        <a href="mailto:support@fractal.co">
                            support@fractal.co
                        </a>{" "}
                        and be sure to include your name, testimonial location,
                        and contact information.
                    </p>
                    <p>
                        To administer prize draws and competitions. We may use
                        your information to administer prize draws and
                        competitions when you elect to participate in
                        competitions.
                    </p>
                    <p>
                        To request feedback. We may use your information to
                        request feedback and to contact you about your use of
                        our Services.
                    </p>
                    <p>
                        To protect our Services. We may use your information as
                        part of our efforts to keep our Services safe and secure
                        (for example, to monitor and prevent fraud).
                    </p>
                    <p>
                        To enforce our terms, conditions and policies for
                        Business Purposes, Legal Reasons and Contractual
                        Reasons.
                    </p>
                    <p>
                        To respond to legal requests and prevent harm. If we
                        receive a subpoena or other legal request, we may need
                        to inspect the data we hold to determine how to respond.
                    </p>
                    <p>
                        To manage user accounts. We may use your information for
                        the purposes of managing our account and keeping it in
                        working order.
                    </p>
                    <p>
                        To deliver services to the user. We may use your
                        information to provide you with the requested service.
                    </p>
                    <p>
                        For other Business Purposes. We may use your information
                        for other Business Purposes, such as data analysis,
                        identifying usage trends, determining the effectiveness
                        of our promotional campaigns, and to evaluate and
                        improve our Services, products, marketing and your
                        experience. We may use and store this information. In
                        either case, this information will exist in aggregated
                        and anonymized form so that it is not associated with
                        individual end users and does not include personal
                        information. We will not use identifiable personal
                        information without your consent.
                    </p>
                </div>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    3. Will Your Information Be Shared With Anyone?
                </p>
                <p>
                    <u>Summary:</u> We only share information with your consent,
                    to comply with laws, to provide you with services, to
                    protect your rights, or to fulfill business obligations.
                </p>
                <div style={{ paddingLeft: 20 }}>
                    <p style={{ fontWeight: "bold" }}>
                        We May Process Or Share Data Based On The Following
                        Legal Basis:
                    </p>
                    <p>
                        <u>Consent:</u> We may process your data if you have
                        given us specific consent to use your personal
                        information for a specific purpose.
                    </p>
                    <p>
                        <u>Legal Obligations:</u> We may disclose your
                        information where we are legally required to do so in
                        order to comply with applicable law, governmental
                        requests, a judicial proceeding, court order, or legal
                        process, such as in response to a court order or a
                        subpoena (including in response to public authorities to
                        meet national security or law enforcement requirements).
                    </p>
                    <p>
                        <u>Vital Interests:</u> We may disclose your information
                        where we believe it is necessary to investigate,
                        prevent, or take action regarding potential violations
                        of our policies, suspected fraud, situations involving
                        potential threats to the safety of any person and
                        illegal activities, or as evidence in litigation in
                        which we are involved.
                    </p>
                    <p>
                        <u>Legitimate Interests:</u> We may process your data
                        when it is reasonably necessary to achieve our
                        legitimate business interests.
                    </p>
                    <p>
                        <u>Performance of a Contract:</u> Where we have entered
                        into a contract with you, we may process your personal
                        information to fulfill the terms of our contract.
                    </p>
                    <p>
                        More specifically, we may need to process your data or
                        share your personal information in the following
                        situations:
                    </p>
                    <p>
                        Vendors, Consultants and Other Third-Party Service
                        Providers. We may share your data with third party
                        vendors, service providers, contractors or agents who
                        perform services for us or on our behalf and require
                        access to such information to do that work. Examples
                        include: payment processing, data analysis, email
                        delivery, hosting services, customer service and
                        marketing efforts. We may allow selected third parties
                        to use tracking technology on the Services, which will
                        enable them to collect data about how you interact with
                        the Services over time. This information may be used to,
                        among other things, analyze and track data, determine
                        the popularity of certain content and better understand
                        online activity. Unless described in this Policy, we do
                        not share, sell, rent or trade any of your information
                        with third parties for their promotional purposes.
                    </p>
                    <p>
                        Business Transfers. We may share or transfer your
                        information in connection with, or during negotiations
                        of, any merger, sale of company assets, financing, or
                        acquisition of all or a portion of our business to
                        another company.
                    </p>
                    <p>
                        Third-Party Advertisers. We may use third-party
                        advertising companies to serve ads when you visit the
                        Services. These companies may use information about your
                        visits to our Website(s) and other websites that are
                        contained in web cookies and other tracking technologies
                        in order to provide personalized advertisements about
                        goods and services.
                    </p>
                </div>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    4. Do We Use Cookies And Other Tracking Technologies?
                </p>
                <p>
                    <u>Summary:</u> We may use cookies and other tracking
                    technologies to collect and store your information.
                </p>
                <p>
                    We may use cookies and similar tracking technologies (like
                    web beacons and pixels) to access or store information.
                    Specific information about how we use such technologies,
                    requesting your consent, and how you can refuse certain
                    cookies is set out in our{" "}
                    <HashLink to="/cookies#top">Cookie Policy</HashLink>.
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    5. How Do We Handle Your Social Logins?
                </p>
                <p>
                    <u>Summary:</u> If you choose to register or log in to our
                    Services using a social media account, we may have access to
                    certain information about you.
                </p>
                <p>
                    Our Services offer you the ability to register and login
                    using your third party social media account details (like
                    your Google or Facebook logins). Where you choose to do
                    this, we will receive your profile information from your
                    social media provider. The profile information we receive
                    may vary depending on the social media provider concerned,
                    but will often include your name, email address, friends
                    list, profile picture and other information you chose to
                    make public.
                </p>
                <p>
                    We will use the information we receive only for the purposes
                    that are described in this privacy policy or that are
                    otherwise made clear to you on the Services. Please note
                    that we do not control, and are not responsible for, other
                    uses of your personal information by your third party social
                    media provider. We recommend that you review their privacy
                    policy to understand how they collect, use and share your
                    personal information, and how you can set your privacy
                    preferences on their sites and applications.
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    6. How Long Do We Keep Your Information?
                </p>
                <p>
                    <u>Summary:</u> We keep your information for as long as
                    necessary to fulfill the purposes outlined in this privacy
                    policy, unless otherwise required by law.
                </p>
                <p>
                    We will only keep your personal information for as long as
                    it is necessary for the purposes set out in this privacy
                    policy, unless a longer retention period is required or
                    permitted by law (for tax, accounting or other legal
                    obligations). None of our business purposes (described in
                    this privacy policy) require us to keep your personal
                    information after you no longer have an account with us.
                    Once we no longer need to process your personal information,
                    we will either delete or anonymize. In some cases it may not
                    be possible to delete or anonymize your data immediately -
                    for example if your personal information has been backed up
                    in an archive. In these cases, we will securely store your
                    personal information and isolate it from any further
                    processing until deletion is possible.
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    7. How Do We Keep Your Information Safe?
                </p>
                <p>
                    <u>Summary:</u> We aim to protect your personal information
                    through a system of organizational and technical security
                    measures.
                </p>
                <p>
                    Although we do our best to protect your personal
                    information, transmission of personal information to and
                    from our Services is at your own risk. We have implemented
                    appropriate technical and organizational security measures
                    designed to protect the security of any personal information
                    we process. However, we cannot guarantee that the Internet
                    itself is 100% secure. Consequently, you should only access
                    the Services within a secure environment.
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    8. Do We Collect Information From Minors?
                </p>
                <p>
                    <u>Summary:</u> We do not knowingly collect data from, or
                    market to, children under 18 years of age.
                </p>
                <p>
                    By using the Services, you represent that you are at least
                    18 years old or that you are the parent or guardian of such
                    a minor and consent to such minor dependent’s use of the
                    Services. If we learn that personal information from users
                    less than 18 years of age has been collected, we will
                    deactivate the account and take reasonable measures to
                    promptly delete such data from our records. If you become
                    aware of any data we have collected from children under age
                    18, please contact us at{" "}
                    <a href="mailto:support@fractal.co">support@fractal.co</a>.
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    9. Advertising And Analytics Services Provided By Others
                </p>
                <p>
                    <u>Summary:</u> We may work with, and share non-personal
                    information with, third parties to serve advertisements on
                    our behalf.
                </p>
                <p>
                    We may allow third parties to serve advertisements on our
                    behalf across the Internet and to provide analytics services
                    in relation to the use of our Services. These entities may
                    use cookies, web beacons and other technologies to collect
                    information about your use of the Services and other
                    websites, including your IP address and information about
                    your web browser and operating system, pages viewed, time
                    spent on pages, links clicked and conversion information.
                    This information may be used to, among other things, analyze
                    and track data, determine the popularity of certain content,
                    deliver advertising and content targeted to your interests
                    on our Services and other websites and better understand
                    your online activity. Please note that we may share
                    aggregated, non-personal information, or personal
                    information in a hashed, non-human readable form, with these
                    third parties. For example, we may engage a third party data
                    provider who may collect web log data from you or place or
                    recognize a unique cookie on your browser to enable you to
                    receive customized ads or content. These cookies may reflect
                    de-identified demographic or other data linked to
                    information you voluntarily provide to us (e.g., an email
                    address) in a hashed, non-human readable form. For more
                    information about interest-based ads, or to opt-out of
                    having your web browsing information used for behavioral
                    advertising purposes, please visit{" "}
                    <a href="https://www.aboutads.info/choices">
                        www.aboutads.info/choices
                    </a>
                    .
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    10. What Are Your Privacy Rights?
                </p>
                <p>
                    <u>Summary:</u> You may review, change, or terminate your
                    account at any time.
                </p>
                <p>
                    If you are resident in the European Economic Area and you
                    believe we are unlawfully processing your personal
                    information, you have the right to complain to your local
                    data protection supervisory authority. You can find their
                    contact details{" "}
                    <a href="http://ec.europa.eu/justice/data-protection/bodies/authorities/index_en.htm">
                        here
                    </a>
                    . If you have questions or comments about your privacy
                    rights, you may email us at{" "}
                    <a href="mailto:support@fractal.co">support@fractal.co</a>.
                </p>
                <div style={{ paddingLeft: 20 }}>
                    <p style={{ fontWeight: "bold" }}>Account Information</p>
                    <p>
                        If you would like to review or change the information in
                        your account, or terminate your account, you can:
                    </p>
                    <p>
                        Log into your account settings and update your user
                        account, or contact us at{" "}
                        <a href="mailto:support@fractal.co">
                            support@fractal.co
                        </a>
                        .
                    </p>
                    <p>
                        Upon your request to terminate your account, we will
                        deactivate or delete your account and information from
                        our active databases. However, some information may be
                        retained in our files to prevent fraud, troubleshoot
                        problems, assist with any investigations, enforce our{" "}
                        <a href="https://www.fractal.co/termsofservice">
                            Terms of Service
                        </a>{" "}
                        and/or comply with legal requirements.
                    </p>
                    <p>
                        <span
                            style={{
                                fontWeight: "bold",
                                display: "block",
                                marginBottom: "1em",
                            }}
                        >
                            Cookies And Similar Technologies
                        </span>
                        Most Web browsers are set to accept cookies by default.
                        You may be able to configure your browser to remove and
                        reject cookies. Removing or rejecting cookies may reduce
                        the effectiveness of certain features of our Services.
                        To opt-out of interest-based advertising by advertisers
                        on our Services, visit{" "}
                        <a href="https://www.aboutads.info/choices">
                            www.aboutads.info/choices
                        </a>
                        .
                    </p>
                    <p>
                        <span
                            style={{
                                fontWeight: "bold",
                                display: "block",
                                marginBottom: "1em",
                            }}
                        >
                            Opting Out Of Email Marketing
                        </span>
                        You can unsubscribe from our marketing email list at any
                        time by clicking on the unsubscribe link in the emails
                        that we send or by contacting us using the details
                        provided below. Subsequently, we will only send
                        service-related emails that are necessary for the
                        administration and use of your account. To otherwise
                        opt-out of marketing emails, you may:
                    </p>
                    <p>
                        Note your preferences when you register an account with
                        the site, access your account settings and update
                        preferences, or contact us by email at{" "}
                        <a href="mailto:support@fractal.co">
                            support@fractal.co
                        </a>
                        .
                    </p>
                </div>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    11. Controls For Do-not-track Features
                </p>
                <p>
                    <u>Summary:</u> You may opt-in to Do-not-track features in
                    your browser, but we will not respond to DNT browser signals
                    unless a standard for online tracking is adopted in the
                    future.
                </p>
                <p>
                    Most web browsers and some mobile operating systems and
                    mobile applications include a Do-Not-Track (“DNT”) feature
                    or setting you can activate to signal your privacy
                    preference not to have data about your online browsing
                    activities monitored and collected. No uniform technology
                    standard for recognizing and implementing DNT signals has
                    been finalized. As such, we do not currently respond to DNT
                    browser signals or any other mechanism that automatically
                    communicates your choice not to be tracked online. If a
                    standard for online tracking is adopted that we must follow
                    in the future, we will inform you about that practice in a
                    revised version of this privacy policy.
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    12. Do California Residents Have Specific Privacy Rights?
                </p>
                <p>
                    <u>Summary:</u> Yes, if you are a resident of California,
                    you are granted specific rights regarding access to your
                    personal information.
                </p>
                <p>
                    California Civil Code Section 1798.83, also known as the
                    “Shine The Light” law, permits our users who are California
                    residents to request and obtain from us, once a year and
                    free of charge, information about categories of personal
                    information (if any) we disclosed to third parties for
                    direct marketing purposes and the names and addresses of all
                    third parties with which we shared personal information in
                    the immediately preceding calendar year. If you are a
                    California resident and would like to make such a request,
                    please submit your request in writing to us using the
                    contact information provided below.
                </p>
                <p>
                    If you are under 18 years of age, reside in California, and
                    have a registered account with the Services, you have the
                    right to request removal of unwanted data that you publicly
                    post on the Services. To request removal of such data,
                    please contact us using the contact information provided
                    below, and include the email address associated with your
                    account and a statement that you reside in California. We
                    will make sure the data is not publicly displayed on the
                    Services, but please be aware that the data may not be
                    completely or comprehensively removed from our systems.
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    13. Do We Make Updates To This Policy?
                </p>
                <p>
                    <u>Summary:</u> Yes, we will update this policy as necessary
                    to stay compliant with relevant laws.
                </p>
                <p>
                    We may update this privacy policy from time to time. The
                    updated version will be indicated by the “Revised” date and
                    the updated version will be effective as soon as it is
                    accessible. If we make material changes to this privacy
                    policy, we may notify you either by prominently posting a
                    notice of such changes on our Services’ platform or by
                    directly sending you a notification. We encourage you to
                    review this privacy policy frequently to be informed of how
                    we are protecting your information.
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    14. How Can You Contact Us About This Policy?
                </p>
                <p>
                    If you have questions or comments about this policy, you may
                    email us at{" "}
                    <a href="mailto:support@fractal.co">support@fractal.co</a>{" "}
                    or by post to:
                </p>
                <p>
                    Fractal Computers, Inc. <br />
                    33 Irving Place <br />
                    New York, NY 10003 <br />
                    United States <br />
                </p>
                <p style={{ fontWeight: "bold", fontSize: 20 }}>
                    15. How Can You Review, Update, Or Delete The Data We
                    Collect From You?
                </p>
                <p>
                    <u>Summary:</u> Please contact us via email to update the
                    data we collect from you.
                </p>
                <p>
                    Based on the laws of some countries, you may have the right
                    to request access to the personal information we collect
                    from you, change that information, or delete it in some
                    circumstances. To request to review, update, or delete your
                    personal information, please submit a request form by
                    emailing the above contact email. We will respond to your
                    request within 30 days.
                </p>
            </div>
        </div>
        <p style={{ fontWeight: "bold", fontSize: 20 }}>
          11. Controls For Do-not-track Features
        </p>
        <p>
          <u>Summary:</u> You may opt-in to Do-not-track features in your
          browser, but we will not respond to DNT browser signals unless a
          standard for online tracking is adopted in the future.
        </p>
        <p>
          Most web browsers and some mobile operating systems and mobile
          applications include a Do-Not-Track (“DNT”) feature or setting you can
          activate to signal your privacy preference not to have data about your
          online browsing activities monitored and collected. No uniform
          technology standard for recognizing and implementing DNT signals has
          been finalized. As such, we do not currently respond to DNT browser
          signals or any other mechanism that automatically communicates your
          choice not to be tracked online. If a standard for online tracking is
          adopted that we must follow in the future, we will inform you about
          that practice in a revised version of this privacy policy.
        </p>
        <p style={{ fontWeight: "bold", fontSize: 20 }}>
          12. Do California Residents Have Specific Privacy Rights?
        </p>
        <p>
          <u>Summary:</u> Yes, if you are a resident of California, you are
          granted specific rights regarding access to your personal information.
        </p>
        <p>
          California Civil Code Section 1798.83, also known as the “Shine The
          Light” law, permits our users who are California residents to request
          and obtain from us, once a year and free of charge, information about
          categories of personal information (if any) we disclosed to third
          parties for direct marketing purposes and the names and addresses of
          all third parties with which we shared personal information in the
          immediately preceding calendar year. If you are a California resident
          and would like to make such a request, please submit your request in
          writing to us using the contact information provided below.
        </p>
        <p>
          If you are under 18 years of age, reside in California, and have a
          registered account with the Services, you have the right to request
          removal of unwanted data that you publicly post on the Services. To
          request removal of such data, please contact us using the contact
          information provided below, and include the email address associated
          with your account and a statement that you reside in California. We
          will make sure the data is not publicly displayed on the Services, but
          please be aware that the data may not be completely or comprehensively
          removed from our systems.
        </p>
        <p style={{ fontWeight: "bold", fontSize: 20 }}>
          13. Do We Make Updates To This Policy?
        </p>
        <p>
          <u>Summary:</u> Yes, we will update this policy as necessary to stay
          compliant with relevant laws.
        </p>
        <p>
          We may update this privacy policy from time to time. The updated
          version will be indicated by the “Revised” date and the updated
          version will be effective as soon as it is accessible. If we make
          material changes to this privacy policy, we may notify you either by
          prominently posting a notice of such changes on our Services’ platform
          or by directly sending you a notification. We encourage you to review
          this privacy policy frequently to be informed of how we are protecting
          your information.
        </p>
        <p style={{ fontWeight: "bold", fontSize: 20 }}>
          14. How Can You Contact Us About This Policy?
        </p>
        <p>
          If you have questions or comments about this policy, you may email us
          at <a href="mailto:support@fractal.co">support@fractal.co</a> or by
          post to:
        </p>
        <p>
          Fractal Computers, Inc. <br />
          Six Landmark Square, 4th Floor <br />
          Stamford, CT 06901 <br />
          United States <br />
        </p>
        <p style={{ fontWeight: "bold", fontSize: 20 }}>
          15. How Can You Review, Update, Or Delete The Data We Collect From
          You?
        </p>
        <p>
          <u>Summary:</u> Please contact us via email to update the data we
          collect from you.
        </p>
        <p>
          Based on the laws of some countries, you may have the right to request
          access to the personal information we collect from you, change that
          information, or delete it in some circumstances. To request to review,
          update, or delete your personal information, please submit a request
          form by emailing the above contact email. We will respond to your
          request within 30 days.
        </p>
      </div>
    </div>
  )
}

export default Privacy
