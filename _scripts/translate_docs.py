#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path


DEFAULT_LOCALES = ("es", "fr", "de", "ja", "it", "ko", "vi", "yue", "el", "hi")

TranslationBundle = dict[str, dict[str, str]]

BUILTIN_TRANSLATIONS: dict[str, dict[str, str]] = {
    "es": {
        "CoolBox Documentation": "Documentación de CoolBox",
        "CoolBox Documentation Portal": "Portal de documentación de CoolBox",
        "Unified entry point for native C++ documentation and extension-specific docs.": "Punto de entrada unificado para la documentación nativa de C++ y la documentación de extensiones.",
        "GitHub releases:": "Versiones de GitHub:",
        "Latest release": "Última versión",
        "All releases": "Todas las versiones",
        "Documentation & Downloads": "Documentación y descargas",
        "Tutorials": "Tutoriales",
        "𓂀 Tutorials": "𓂀 Tutoriales",
        "Publications": "Publicaciones",
        "இ Publications": "இ Publicaciones",
        "Publication records with metadata and downloadable PDFs.": "Registros de publicaciones con metadatos y archivos PDF descargables.",
        "Recent publication · updated ": "Publicación reciente · actualizada ",
        "Metadata records and downloadable PDFs for CoolBox publications.": "Registros de metadatos y archivos PDF descargables para las publicaciones de CoolBox.",
        "Browse metadata and blank starter PDFs for publications.": "Explora metadatos y PDF iniciales en blanco para publicaciones.",
        "Open publications": "Abrir publicaciones",
        "Publication details will be added later.": "Los detalles de la publicación se añadirán más adelante.",
        "Abstract coming soon.": "Resumen próximamente.",
        "No publications are available yet.": "Todavía no hay publicaciones disponibles.",
        "No recent publications are available yet.": "Todavía no hay publicaciones recientes disponibles.",
        "Latest tutorial post · updated ": "Última publicación del tutorial · actualizado ",
        "No recent tutorial posts are available yet.": "Todavía no hay publicaciones recientes de tutoriales.",
        "Back to docs index": "Volver al índice de documentación",
        "CoolBox Tutorials": "Tutoriales de CoolBox",
        "Static tutorials generated from `.tut` files with support for headings, media, links, and tags.": "Tutoriales estáticos generados a partir de archivos `.tut` con soporte para encabezados, medios, enlaces y etiquetas.",
        "Back to tutorials": "Volver a los tutoriales",
        "Tags": "Etiquetas",
        "No tags defined yet.": "Todavía no se han definido etiquetas.",
        "Generated from ": "Generado a partir de ",
        "Language translations": "Traducciones de idioma",
        "Browse localized copies of the generated docs portal.": "Explora copias localizadas del portal de documentación generado.",
        "Languages": "Idiomas",
        "𓁿 Meandering LLC © 2026": "𓁿 Meandering LLC © 2026",
    },
    "fr": {
        "CoolBox Documentation": "Documentation CoolBox",
        "CoolBox Documentation Portal": "Portail de documentation CoolBox",
        "Unified entry point for native C++ documentation and extension-specific docs.": "Point d'entrée unifié pour la documentation C++ native et la documentation spécifique aux extensions.",
        "GitHub releases:": "Versions GitHub :",
        "Latest release": "Dernière version",
        "All releases": "Toutes les versions",
        "Documentation & Downloads": "Documentation et téléchargements",
        "Tutorials": "Tutoriels",
        "𓂀 Tutorials": "𓂀 Tutoriels",
        "Publications": "Publications",
        "இ Publications": "இ Publications",
        "Publication records with metadata and downloadable PDFs.": "Dossiers de publication avec métadonnées et PDF téléchargeables.",
        "Recent publication · updated ": "Publication récente · mise à jour le ",
        "Metadata records and downloadable PDFs for CoolBox publications.": "Enregistrements de métadonnées et PDF téléchargeables pour les publications CoolBox.",
        "Browse metadata and blank starter PDFs for publications.": "Parcourez les métadonnées et les PDF de départ vierges pour les publications.",
        "Open publications": "Ouvrir les publications",
        "Publication details will be added later.": "Les détails de la publication seront ajoutés plus tard.",
        "Abstract coming soon.": "Résumé à venir.",
        "No publications are available yet.": "Aucune publication n'est encore disponible.",
        "No recent publications are available yet.": "Aucune publication récente n'est encore disponible.",
        "Latest tutorial post · updated ": "Dernier tutoriel · mis à jour le ",
        "No recent tutorial posts are available yet.": "Aucun tutoriel récent n'est encore disponible.",
        "Back to docs index": "Retour à l'index de la documentation",
        "CoolBox Tutorials": "Tutoriels CoolBox",
        "Static tutorials generated from `.tut` files with support for headings, media, links, and tags.": "Tutoriels statiques générés à partir de fichiers `.tut` avec prise en charge des titres, médias, liens et étiquettes.",
        "Back to tutorials": "Retour aux tutoriels",
        "Tags": "Étiquettes",
        "No tags defined yet.": "Aucune étiquette n'est encore définie.",
        "Generated from ": "Généré à partir de ",
        "Language translations": "Traductions linguistiques",
        "Browse localized copies of the generated docs portal.": "Parcourez des copies localisées du portail de documentation généré.",
        "Languages": "Langues",
        "𓁿 Meandering LLC © 2026": "𓁿 Meandering LLC © 2026",
    },
    "de": {
        "CoolBox Documentation": "CoolBox-Dokumentation",
        "CoolBox Documentation Portal": "CoolBox-Dokumentationsportal",
        "Unified entry point for native C++ documentation and extension-specific docs.": "Einheitlicher Einstiegspunkt für native C++-Dokumentation und erweiterungsspezifische Dokumentation.",
        "GitHub releases:": "GitHub-Releases:",
        "Latest release": "Neueste Version",
        "All releases": "Alle Versionen",
        "Documentation & Downloads": "Dokumentation und Downloads",
        "Tutorials": "Tutorials",
        "𓂀 Tutorials": "𓂀 Tutorials",
        "Publications": "Publikationen",
        "இ Publications": "இ Publikationen",
        "Publication records with metadata and downloadable PDFs.": "Publikationsdatensätze mit Metadaten und herunterladbaren PDFs.",
        "Recent publication · updated ": "Neueste Publikation · aktualisiert ",
        "Metadata records and downloadable PDFs for CoolBox publications.": "Metadatensätze und herunterladbare PDFs für CoolBox-Publikationen.",
        "Browse metadata and blank starter PDFs for publications.": "Metadaten und leere Starter-PDFs für Publikationen durchsuchen.",
        "Open publications": "Publikationen öffnen",
        "Publication details will be added later.": "Die Publikationsdetails werden später hinzugefügt.",
        "Abstract coming soon.": "Zusammenfassung folgt.",
        "No publications are available yet.": "Derzeit sind noch keine Publikationen verfügbar.",
        "No recent publications are available yet.": "Derzeit sind noch keine aktuellen Publikationen verfügbar.",
        "Latest tutorial post · updated ": "Neuester Tutorial-Beitrag · aktualisiert ",
        "No recent tutorial posts are available yet.": "Derzeit sind noch keine aktuellen Tutorial-Beiträge verfügbar.",
        "Back to docs index": "Zurück zum Dokumentationsindex",
        "CoolBox Tutorials": "CoolBox-Tutorials",
        "Static tutorials generated from `.tut` files with support for headings, media, links, and tags.": "Statische Tutorials, die aus `.tut`-Dateien mit Unterstützung für Überschriften, Medien, Links und Tags erzeugt werden.",
        "Back to tutorials": "Zurück zu den Tutorials",
        "Tags": "Tags",
        "No tags defined yet.": "Es sind noch keine Tags definiert.",
        "Generated from ": "Generiert aus ",
        "Language translations": "Sprachübersetzungen",
        "Browse localized copies of the generated docs portal.": "Lokalisierte Kopien des generierten Dokumentationsportals durchsuchen.",
        "Languages": "Sprachen",
        "𓁿 Meandering LLC © 2026": "𓁿 Meandering LLC © 2026",
    },
    "ja": {
        "CoolBox Documentation": "CoolBox ドキュメント",
        "CoolBox Documentation Portal": "CoolBox ドキュメントポータル",
        "Unified entry point for native C++ documentation and extension-specific docs.": "ネイティブ C++ ドキュメントと拡張機能別ドキュメントの統合入口です。",
        "GitHub releases:": "GitHub リリース:",
        "Latest release": "最新リリース",
        "All releases": "すべてのリリース",
        "Documentation & Downloads": "ドキュメントとダウンロード",
        "Tutorials": "チュートリアル",
        "𓂀 Tutorials": "𓂀 チュートリアル",
        "Publications": "出版物",
        "இ Publications": "இ 出版物",
        "Publication records with metadata and downloadable PDFs.": "メタデータとダウンロード可能な PDF を含む出版物レコードです。",
        "Recent publication · updated ": "最近の出版物・更新日 ",
        "Metadata records and downloadable PDFs for CoolBox publications.": "CoolBox 出版物のメタデータ記録とダウンロード可能な PDF です。",
        "Browse metadata and blank starter PDFs for publications.": "出版物向けのメタデータと空のスターター PDF を参照できます。",
        "Open publications": "出版物を開く",
        "Publication details will be added later.": "出版物の詳細は後で追加されます。",
        "Abstract coming soon.": "要約は近日公開です。",
        "No publications are available yet.": "利用可能な出版物はまだありません。",
        "No recent publications are available yet.": "最近の出版物はまだありません。",
        "Latest tutorial post · updated ": "最新チュートリアル投稿・更新日 ",
        "No recent tutorial posts are available yet.": "最近のチュートリアル投稿はまだありません。",
        "Back to docs index": "ドキュメント索引に戻る",
        "CoolBox Tutorials": "CoolBox チュートリアル",
        "Static tutorials generated from `.tut` files with support for headings, media, links, and tags.": "見出し、メディア、リンク、タグに対応した `.tut` ファイルから生成された静的チュートリアルです。",
        "Back to tutorials": "チュートリアルに戻る",
        "Tags": "タグ",
        "No tags defined yet.": "まだタグは定義されていません。",
        "Generated from ": "生成元: ",
        "Language translations": "言語翻訳",
        "Browse localized copies of the generated docs portal.": "生成されたドキュメントポータルのローカライズ版を参照できます。",
        "Languages": "言語",
        "𓁿 Meandering LLC © 2026": "𓁿 Meandering LLC © 2026",
    },
    "it": {},
    "ko": {},
    "vi": {
        "CoolBox Documentation": "Tai lieu CoolBox",
        "CoolBox Documentation Portal": "Cong thong tin tai lieu CoolBox",
        "Unified entry point for native C++ documentation and extension-specific docs.": "Diem truy cap hop nhat cho tai lieu C++ goc va tai lieu rieng cho tung phan mo rong.",
        "GitHub releases:": "Ban phat hanh GitHub:",
        "Latest release": "Ban moi nhat",
        "All releases": "Tat ca cac ban phat hanh",
        "Documentation & Downloads": "Tai lieu va tai xuong",
        "Tutorials": "Huong dan",
        "𓂀 Tutorials": "𓂀 Huong dan",
        "Publications": "An pham",
        "இ Publications": "இ An pham",
        "Publication records with metadata and downloadable PDFs.": "Ban ghi an pham voi metadata va file PDF co the tai xuong.",
        "Recent publication · updated ": "An pham gan day · cap nhat ",
        "Metadata records and downloadable PDFs for CoolBox publications.": "Ban ghi metadata va file PDF co the tai xuong cho cac an pham CoolBox.",
        "Browse metadata and blank starter PDFs for publications.": "Xem metadata va cac file PDF mau trong cho an pham.",
        "Open publications": "Mo an pham",
        "Publication details will be added later.": "Thong tin chi tiet ve an pham se duoc bo sung sau.",
        "Abstract coming soon.": "Tom tat se som duoc cap nhat.",
        "No publications are available yet.": "Chua co an pham nao san sang.",
        "No recent publications are available yet.": "Chua co an pham gan day nao.",
        "Latest tutorial post · updated ": "Bai huong dan moi nhat · cap nhat ",
        "No recent tutorial posts are available yet.": "Chua co bai huong dan gan day nao.",
        "Back to docs index": "Quay lai muc luc tai lieu",
        "CoolBox Tutorials": "Huong dan CoolBox",
        "Static tutorials generated from `.tut` files with support for headings, media, links, and tags.": "Cac huong dan tinh duoc tao tu tep `.tut`, ho tro tieu de, media, lien ket va the.",
        "Back to tutorials": "Quay lai huong dan",
        "Tags": "The",
        "No tags defined yet.": "Chua co the nao duoc dinh nghia.",
        "Generated from ": "Duoc tao tu ",
        "Language translations": "Ban dich ngon ngu",
        "Browse localized copies of the generated docs portal.": "Xem cac ban da duoc ban dia hoa cua cong thong tin tai lieu da tao.",
        "Languages": "Ngon ngu",
        "𓁿 Meandering LLC © 2026": "𓁿 Meandering LLC © 2026",
    },
    "yue": {
        "CoolBox Documentation": "CoolBox 文件",
        "CoolBox Documentation Portal": "CoolBox 文件入口",
        "Unified entry point for native C++ documentation and extension-specific docs.": "原生 C++ 文件同各個擴充文件嘅統一入口。",
        "GitHub releases:": "GitHub 發佈：",
        "Latest release": "最新版本",
        "All releases": "所有版本",
        "Documentation & Downloads": "文件同下載",
        "Tutorials": "教學",
        "𓂀 Tutorials": "𓂀 教學",
        "Publications": "出版項目",
        "இ Publications": "இ 出版項目",
        "Publication records with metadata and downloadable PDFs.": "附有中繼資料同可下載 PDF 嘅出版記錄。",
        "Recent publication · updated ": "近期出版項目 · 更新於 ",
        "Metadata records and downloadable PDFs for CoolBox publications.": "CoolBox 出版項目嘅中繼資料記錄同可下載 PDF。",
        "Browse metadata and blank starter PDFs for publications.": "瀏覽出版項目嘅中繼資料同空白 PDF 範本。",
        "Open publications": "開啟出版項目",
        "Publication details will be added later.": "出版詳情稍後補上。",
        "Abstract coming soon.": "摘要即將提供。",
        "No publications are available yet.": "暫時未有出版項目。",
        "No recent publications are available yet.": "暫時未有近期出版項目。",
        "Latest tutorial post · updated ": "最新教學文章 · 更新於 ",
        "No recent tutorial posts are available yet.": "暫時未有近期教學文章。",
        "Back to docs index": "返回文件首頁",
        "CoolBox Tutorials": "CoolBox 教學",
        "Static tutorials generated from `.tut` files with support for headings, media, links, and tags.": "由 `.tut` 檔案產生嘅靜態教學，支援標題、媒體、連結同標籤。",
        "Back to tutorials": "返回教學",
        "Tags": "標籤",
        "No tags defined yet.": "暫時未有標籤。",
        "Generated from ": "產生自 ",
        "Language translations": "語言翻譯",
        "Browse localized copies of the generated docs portal.": "瀏覽已產生文件入口嘅本地化版本。",
        "Languages": "語言",
        "𓁿 Meandering LLC © 2026": "𓁿 Meandering LLC © 2026",
    },
    "el": {
        "CoolBox Documentation": "Τεκμηρίωση CoolBox",
        "CoolBox Documentation Portal": "Πύλη τεκμηρίωσης CoolBox",
        "Unified entry point for native C++ documentation and extension-specific docs.": "Ενιαίο σημείο εισόδου για την εγγενή τεκμηρίωση C++ και την τεκμηρίωση των επεκτάσεων.",
        "GitHub releases:": "Εκδόσεις GitHub:",
        "Latest release": "Τελευταία έκδοση",
        "All releases": "Όλες οι εκδόσεις",
        "Documentation & Downloads": "Τεκμηρίωση και λήψεις",
        "Tutorials": "Οδηγοί",
        "𓂀 Tutorials": "𓂀 Οδηγοί",
        "Publications": "Δημοσιεύσεις",
        "இ Publications": "இ Δημοσιεύσεις",
        "Publication records with metadata and downloadable PDFs.": "Εγγραφές δημοσιεύσεων με μεταδεδομένα και PDF για λήψη.",
        "Recent publication · updated ": "Πρόσφατη δημοσίευση · ενημερώθηκε ",
        "Metadata records and downloadable PDFs for CoolBox publications.": "Εγγραφές μεταδεδομένων και PDF για λήψη για τις δημοσιεύσεις CoolBox.",
        "Browse metadata and blank starter PDFs for publications.": "Περιηγηθείτε σε μεταδεδομένα και κενά αρχικά PDF για δημοσιεύσεις.",
        "Open publications": "Άνοιγμα δημοσιεύσεων",
        "Publication details will be added later.": "Οι λεπτομέρειες της δημοσίευσης θα προστεθούν αργότερα.",
        "Abstract coming soon.": "Η περίληψη θα προστεθεί σύντομα.",
        "No publications are available yet.": "Δεν υπάρχουν ακόμη διαθέσιμες δημοσιεύσεις.",
        "No recent publications are available yet.": "Δεν υπάρχουν ακόμη πρόσφατες δημοσιεύσεις.",
        "Latest tutorial post · updated ": "Τελευταίος οδηγός · ενημερώθηκε ",
        "No recent tutorial posts are available yet.": "Δεν υπάρχουν ακόμη πρόσφατοι οδηγοί.",
        "Back to docs index": "Επιστροφή στο ευρετήριο τεκμηρίωσης",
        "CoolBox Tutorials": "Οδηγοί CoolBox",
        "Static tutorials generated from `.tut` files with support for headings, media, links, and tags.": "Στατικοί οδηγοί που δημιουργούνται από αρχεία `.tut`, με υποστήριξη για επικεφαλίδες, πολυμέσα, συνδέσμους και ετικέτες.",
        "Back to tutorials": "Επιστροφή στους οδηγούς",
        "Tags": "Ετικέτες",
        "No tags defined yet.": "Δεν έχουν οριστεί ακόμη ετικέτες.",
        "Generated from ": "Δημιουργήθηκε από ",
        "Language translations": "Μεταφράσεις γλωσσών",
        "Browse localized copies of the generated docs portal.": "Περιηγηθείτε σε τοπικοποιημένα αντίγραφα της παραγόμενης πύλης τεκμηρίωσης.",
        "Languages": "Γλώσσες",
        "𓁿 Meandering LLC © 2026": "𓁿 Meandering LLC © 2026",
    },
    "hi": {
        "CoolBox Documentation": "CoolBox प्रलेखन",
        "CoolBox Documentation Portal": "CoolBox प्रलेखन पोर्टल",
        "Unified entry point for native C++ documentation and extension-specific docs.": "मूल C++ प्रलेखन और एक्सटेंशन-विशिष्ट दस्तावेज़ों के लिए एकीकृत प्रवेश बिंदु।",
        "GitHub releases:": "GitHub रिलीज़:",
        "Latest release": "नवीनतम रिलीज़",
        "All releases": "सभी रिलीज़",
        "Documentation & Downloads": "प्रलेखन और डाउनलोड",
        "Tutorials": "ट्यूटोरियल",
        "𓂀 Tutorials": "𓂀 ट्यूटोरियल",
        "Publications": "प्रकाशन",
        "இ Publications": "இ प्रकाशन",
        "Publication records with metadata and downloadable PDFs.": "मेटाडेटा और डाउनलोड करने योग्य PDF सहित प्रकाशन अभिलेख।",
        "Recent publication · updated ": "हाल का प्रकाशन · अद्यतन ",
        "Metadata records and downloadable PDFs for CoolBox publications.": "CoolBox प्रकाशनों के लिए मेटाडेटा अभिलेख और डाउनलोड करने योग्य PDF।",
        "Browse metadata and blank starter PDFs for publications.": "प्रकाशनों के लिए मेटाडेटा और खाली प्रारंभिक PDF देखें।",
        "Open publications": "प्रकाशन खोलें",
        "Publication details will be added later.": "प्रकाशन का विवरण बाद में जोड़ा जाएगा।",
        "Abstract coming soon.": "सारांश शीघ्र उपलब्ध होगा।",
        "No publications are available yet.": "अभी तक कोई प्रकाशन उपलब्ध नहीं है।",
        "No recent publications are available yet.": "अभी तक कोई हालिया प्रकाशन उपलब्ध नहीं है।",
        "Latest tutorial post · updated ": "नवीनतम ट्यूटोरियल पोस्ट · अद्यतन ",
        "No recent tutorial posts are available yet.": "अभी तक कोई हालिया ट्यूटोरियल पोस्ट उपलब्ध नहीं है।",
        "Back to docs index": "दस्तावेज़ सूची पर वापस जाएँ",
        "CoolBox Tutorials": "CoolBox ट्यूटोरियल",
        "Static tutorials generated from `.tut` files with support for headings, media, links, and tags.": "`.tut` फ़ाइलों से उत्पन्न स्थिर ट्यूटोरियल, जिनमें शीर्षक, मीडिया, लिंक और टैग का समर्थन है।",
        "Back to tutorials": "ट्यूटोरियल पर वापस जाएँ",
        "Tags": "टैग",
        "No tags defined yet.": "अभी तक कोई टैग परिभाषित नहीं है।",
        "Generated from ": "से उत्पन्न ",
        "Language translations": "भाषा अनुवाद",
        "Browse localized copies of the generated docs portal.": "उत्पन्न प्रलेखन पोर्टल की स्थानीयकृत प्रतियाँ देखें।",
        "Languages": "भाषाएँ",
        "𓁿 Meandering LLC © 2026": "𓁿 Meandering LLC © 2026",
    },
}

LANGUAGE_LABELS = {
    "en": "English",
    "es": "Espanol",
    "fr": "Francais",
    "de": "Deutsch",
    "ja": "日本語",
    "it": "Italiano",
    "ko": "한국어",
    "vi": "Tieng Viet",
    "yue": "廣東話",
    "el": "Ελληνικά",
    "hi": "हिन्दी",
}

LANGUAGE_SWITCHER_TITLES = {
    "en": "Language translations",
    "es": "Traducciones de idioma",
    "fr": "Traductions linguistiques",
    "de": "Sprachübersetzungen",
    "ja": "言語翻訳",
}


def empty_bundle() -> TranslationBundle:
    return {"phrases": {}, "paragraphs": {}}


def normalize_locale_mapping(mapping: object) -> TranslationBundle:
    bundle = empty_bundle()
    if not isinstance(mapping, dict):
        return bundle

    if "phrases" in mapping or "paragraphs" in mapping:
        phrases = mapping.get("phrases", {})
        paragraphs = mapping.get("paragraphs", {})
        if isinstance(phrases, dict):
            bundle["phrases"] = {str(key): str(value) for key, value in phrases.items()}
        if isinstance(paragraphs, dict):
            bundle["paragraphs"] = {str(key): str(value) for key, value in paragraphs.items()}
        return bundle

    bundle["phrases"] = {str(key): str(value) for key, value in mapping.items()}
    return bundle


def load_catalog_translations(config_path: Path) -> dict[str, TranslationBundle]:
    if not config_path.exists():
        return {}

    payload = json.loads(config_path.read_text(encoding="utf-8"))
    catalog: dict[str, TranslationBundle] = {}

    for section in ("phrases", "paragraphs"):
        entries = payload.get(section, {})
        if not isinstance(entries, dict):
            continue

        for _, translations in entries.items():
            if not isinstance(translations, dict):
                continue
            english = translations.get("en")
            if not english:
                continue

            for locale, translated in translations.items():
                if locale == "en" or not translated:
                    continue
                catalog.setdefault(locale, empty_bundle())
                catalog[locale][section][str(english)] = str(translated)

    return catalog


def load_custom_translations(config_path: Path) -> dict[str, TranslationBundle]:
    if not config_path.exists():
        return {}
    payload = json.loads(config_path.read_text(encoding="utf-8"))
    custom: dict[str, TranslationBundle] = {}
    for locale, mapping in payload.items():
        custom[str(locale)] = normalize_locale_mapping(mapping)
    return custom


def merged_translations(config_path: Path, catalog_path: Path | None = None) -> dict[str, TranslationBundle]:
    translations = {
        locale: {"phrases": mapping.copy(), "paragraphs": {}}
        for locale, mapping in BUILTIN_TRANSLATIONS.items()
    }
    if catalog_path is not None:
        for locale, mapping in load_catalog_translations(catalog_path).items():
            translations.setdefault(locale, empty_bundle())
            translations[locale]["phrases"].update(mapping["phrases"])
            translations[locale]["paragraphs"].update(mapping["paragraphs"])
    for locale, mapping in load_custom_translations(config_path).items():
        translations.setdefault(locale, empty_bundle())
        translations[locale]["phrases"].update(mapping["phrases"])
        translations[locale]["paragraphs"].update(mapping["paragraphs"])
    return translations


def apply_replacements(text: str, replacements: TranslationBundle, locale: str) -> str:
    for source, target in sorted(replacements["paragraphs"].items(), key=lambda item: len(item[0]), reverse=True):
        text = text.replace(source, target)
    for source, target in sorted(replacements["phrases"].items(), key=lambda item: len(item[0]), reverse=True):
        text = text.replace(source, target)
    text = text.replace('<html lang="en">', f'<html lang="{locale}">')
    text = text.replace("<html lang=\"en\">", f"<html lang=\"{locale}\">")
    return text


def translate_tree(source_dir: Path, target_dir: Path, locale: str, replacements: TranslationBundle) -> None:
    if target_dir.exists():
        shutil.rmtree(target_dir)
    shutil.copytree(source_dir, target_dir, ignore=shutil.ignore_patterns(*DEFAULT_LOCALES, ".git", ".github"))

    for html_file in target_dir.rglob("*.html"):
        html = html_file.read_text(encoding="utf-8")
        html_file.write_text(apply_replacements(html, replacements, locale), encoding="utf-8")


def inject_language_switcher(index_path: Path, locales: list[str], current_locale: str = "en") -> None:
    if not index_path.exists():
        return

    html = index_path.read_text(encoding="utf-8")
    if "language-switcher" in html:
        return

    all_locales = ["en", *[locale for locale in locales if locale != "en"]]
    items = []
    for locale in all_locales:
        if current_locale == "en":
            href = "index.html" if locale == "en" else f"{locale}/index.html"
        else:
            href = "../index.html" if locale == "en" else f"../{locale}/index.html"
        css_class = "language-link active" if locale == current_locale else "language-link"
        items.append(f'<a class="{css_class}" href="{href}">{LANGUAGE_LABELS.get(locale, locale)}</a>')

        block = (
            '<div class="footer-language-switcher" aria-label="Languages">'
            '<span class="footer-language-switcher-mark">𖧼</span>'
        '<span class="footer-language-switcher-bracket">[</span>'
        f'{", ".join(items)}'
        '<span class="footer-language-switcher-bracket">]</span>'
            '</div>'
        )

    style_marker = "footer { margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; font-size: 0.95rem; }"
    if style_marker in html:
        html = html.replace(
            style_marker,
            style_marker
            + "\n    footer { display: flex; align-items: center; justify-content: space-between; gap: 1rem; flex-wrap: wrap; }"
            + "\n    .footer-language-switcher { margin-left: auto; display: flex; gap: 0.2rem; flex-wrap: wrap; justify-content: flex-end; align-items: center; }"
            + "\n    .footer-language-switcher-mark { color: #475569; font-weight: 700; }"
            + "\n    .footer-language-switcher-bracket { color: #475569; font-weight: 700; }"
            + "\n    .footer-language-switcher a { font-weight: 600; white-space: nowrap; }"
            + "\n    .footer-language-switcher a.active { color: #000000; }"
        )

    html = html.replace("</footer>", "    " + block + "\n    </footer>", 1)

    index_path.write_text(html, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate localized copies of the built documentation site.")
    parser.add_argument("site_dir", help="Path to the generated site directory")
    parser.add_argument("--locales", nargs="*", default=list(DEFAULT_LOCALES), help="Locales to generate")
    parser.add_argument("--config", default="config/docs_translations.json", help="Optional JSON file with extra translation mappings")
    parser.add_argument("--catalog", default="config/docs_phrase_catalog.json", help="Optional JSON file with UUID-based phrase and paragraph translations")
    args = parser.parse_args()

    print("[translate_docs.py] Starting translation.")
    site_dir = Path(args.site_dir).resolve()
    if not site_dir.exists():
        raise FileNotFoundError(f"Site directory not found: {site_dir}")

    config_path = (site_dir.parent / args.config).resolve() if not Path(args.config).is_absolute() else Path(args.config)
    catalog_path = (site_dir.parent / args.catalog).resolve() if not Path(args.catalog).is_absolute() else Path(args.catalog)
    translations = merged_translations(config_path, catalog_path)
    locales = [locale for locale in args.locales if locale in translations]

    for locale in locales:
        print(f"[translate_docs.py] creating local {locale}")
        translate_tree(site_dir, site_dir / locale, locale, translations[locale])

    inject_language_switcher(site_dir / "index.html", locales, "en")
    for locale in locales:
        print(f"[translate_docs.py] language for localization {locale}")
        inject_language_switcher(site_dir / locale / "index.html", locales, locale)
    print(f"[translate_docs.py] Generated localized docs for: {', '.join(locales)}")


if __name__ == "__main__":
    main()