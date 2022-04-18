#!/usr/bin/env python3

# This script is used to generate the typescript array with the list of languages supported
# by Linux, as well as a dictionary with the default dialect for those languages that have
# regional variations. For these languages, we pick the dialect used in the country where
# there are most speakers.

# This script must be run on Linux! You should then copy the array and the dictionary into
# the frontend/client-applications/src/constants/linuxLanguages.ts

import os
import sys
import subprocess

# fmt: off
language_to_default_dialect = {
    "aa":   "DJ",   # Afar, spoken in Djibouti, Eritrea, and the Afar Region of Ethiopia.
                    # Most speakers live in Djibouti.
    "ar":   "EG",   # Arabic. Country with most speakers of Standard Arabic is Egypt.
    "az":   "AZ",   # Azerbaijani
    "ber":  "MA",   # Berber language, spoken in Morocco, among other countries
    "bho":  "IN",   # Bhojpuri, spoken in India and Nepal
    "bn":   "BD",   # Bengali, spoken in Bangladesh and India
    "bo":   "CN",   # Tibetan, spoken in China and India,
    "ca":   "ES",   # Catalan, spoken chiefly in Spain
    "de":   "DE",   # German
    "el":   "GR",   # Greek
    "en":   "US",   # English
    "es":   "ES",   # Spanish
    "eu":   "FR",   # Basque, spoken in France and Spain
    "fr":   "FR",   # France
    "fy":   "NL",   # Western Frisian. Spoken mainly in the Netherlands
    "gez":  "ET",   # Ge'ez, Ethiopian Semitic language.
    "it":   "IT",   # Italian
    "li":   "NL",   # Limburgish, spoken mainly in the Netherlands
    "mai":  "NP",   # Maithili. Most speakers live in Nepal
    "nds":  "DE",   # Low German / Low Saxon. Most speakers live in Germany
    "niu":  "NZ",   # Niuean. Most speakers live in New Zealand
    "nl":   "NL",   # Dutch
    "om":   "ET",   # Oromo, most speakers live in Ethiopia
    "pa":   "PK",   # Pakistani
    "pap":  "CW",   # Papiamento, spoken mainly in Curaçao
    "pt":   "PT",   # Portuguese
    "ru":   "RU",   # Russian, spoken mainly in Russia
    "sd":   "IN",   # Sindhi, spoken mainly in Pakistan
    "so":   "SO",   # Somali, spoken mainly in Somalia
    "sq":   "AL",   # Albanian
    "sr":   "RS",   # Serbian
    "sv":   "SE",   # Swedish
    "sw":   "TZ",   # Swahili. Country with most speakers is Tanzania
    "ta":   "LK",   # Tamil. Country with most speakers is Sri Lanka
    "ti":   "ET",   # Tigrinya. Country with most speakers is Eritrea
    "tr":   "TR",   # Turkish
    "ur":   "PK",   # Urdu. Country with most speakers is Pakistan
    "zh":   "CN",   # Chinese
}
# fmt: on


def populate_language_to_dialect(locales):

    language_to_dialect = {}

    # Remove part after . and @
    locales = sorted(
        set([x.split(".")[0].split("@")[0] for x in locales if x != "C" and x != "POSIX"])
    )
    locales_string = ", ".join([f"'{loc}'" for loc in locales])
    # Print list of supported <language>_<REGION> combinations
    print(f"export const linuxLanguages: string[] = [{locales_string}]")

    language_to_dialect = {}
    for loc in locales:
        split_locale = loc.split("_")
        if len(split_locale) == 2:
            lang, dialect = split_locale
            if lang in language_to_dialect:
                language_to_dialect[lang].append(dialect)
            else:
                language_to_dialect[lang] = [dialect]
    for language in language_to_dialect:
        if len(language_to_dialect[language]) == 1:
            language_to_dialect[language] = language_to_dialect[language][0]
        # For those languages with multiple dialects, we need to pick a default one
        if len(language_to_dialect[language]) > 1:
            if language in language_to_default_dialect:
                language_to_dialect[language] = language_to_default_dialect[language]

    # Print dictionary of supported <language>_<DEFAULT REGION> combinations
    language_to_dialect_list = [f"'{key}':'{value}'" for key, value in language_to_dialect.items()]
    language_to_dialect_string = ", ".join(language_to_dialect_list)
    print()
    print(
        f"export const linuxLanguageToDefaultRegion: {{ [key: string]: string }} = {{{language_to_dialect_string}}}"
    )


def get_chrome_languages():
    # Languages below obtained by enabling all languages on Chrome (from Settings -> Advanced Settings -> Languages)
    # on Linux and then extracting the list of languages from .config/google-chrome/Default/Preferences
    chrome_languages = "de,it,en-US,en,af,sq,de-DE,de-AT,de-LI,de-CH,am,en-ZA,en-AU,en-CA,en-IN,en-NZ,en-GB-oxendict,en-GB,ar,an,hy,ast,az,eu,bn,be,my,bs,br,bg,ca,ceb,chr,ny,zh,zh-HK,zh-CN,zh-TW,si,ko,co,ht,hr,da,es,es-419,es-AR,es-CL,es-CO,es-CR,es-ES,es-US,es-HN,es-MX,es-PE,es-UY,es-VE,eo,et,fo,fil,fi,fr,fr-CA,fr-FR,fr-CH,gd,gl,cy,ka,gu,el,gn,fy,ha,haw,he,hi,hmn,hu,ig,id,ia,ga,is,it-IT,it-CH,ja,jv,kn,kk,km,rw,ky,ku,lo,la,lv,ln,lt,lb,mk,ms,ml,mg,mt,mi,mr,mn,nl,ne,no,nb,nn,oc,or,om,ug,ur,uz,ps,pa,fa,pl,pt,pt-BR,pt-PT,qu,rm,ro,mo,ru,sm,sr,sh,sn,sd,sk,sl,so,ckb,st,su,sv,sw,tg,ta,tt,cs,te,th,ti,to,tn,tr,tk,tw,uk,vi,wa,wo,xh,yi,yo,zu"
    chrome_languages = ", ".join([f"'{loc}'" for loc in chrome_languages.split(",")])
    print()
    print(f"export const chromeLanguages: string[] = [{chrome_languages}]")


if __name__ == "__main__":
    # Get list of available languages
    subproc_handle = subprocess.Popen("locale -a", shell=True, stdout=subprocess.PIPE)
    subprocess_stdout = subproc_handle.stdout.readlines()
    locales = [line.decode("utf-8").strip() for line in subprocess_stdout]

    populate_language_to_dialect(locales)

    get_chrome_languages()
