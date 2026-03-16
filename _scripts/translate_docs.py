#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path


DEFAULT_LOCALES = ("es", "fr", "de", "ja")

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
        "☉ Tutorials": "☉ Tutoriales",
        "Publications": "Publicaciones",
        "இ Publications": "இ Publicaciones",
        "Publication records with metadata and downloadable PDFs.": "Registros de publicaciones con metadatos y archivos PDF descargables.",
        "Metadata records and downloadable PDFs for CoolBox publications.": "Registros de metadatos y archivos PDF descargables para las publicaciones de CoolBox.",
        "Browse metadata and blank starter PDFs for publications.": "Explora metadatos y PDF iniciales en blanco para publicaciones.",
        "Open publications": "Abrir publicaciones",
        "Publication details will be added later.": "Los detalles de la publicación se añadirán más adelante.",
        "Abstract coming soon.": "Resumen próximamente.",
        "No publications are available yet.": "Todavía no hay publicaciones disponibles.",
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
        "☉ Tutorials": "☉ Tutoriels",
        "Publications": "Publications",
        "இ Publications": "இ Publications",
        "Publication records with metadata and downloadable PDFs.": "Dossiers de publication avec métadonnées et PDF téléchargeables.",
        "Metadata records and downloadable PDFs for CoolBox publications.": "Enregistrements de métadonnées et PDF téléchargeables pour les publications CoolBox.",
        "Browse metadata and blank starter PDFs for publications.": "Parcourez les métadonnées et les PDF de départ vierges pour les publications.",
        "Open publications": "Ouvrir les publications",
        "Publication details will be added later.": "Les détails de la publication seront ajoutés plus tard.",
        "Abstract coming soon.": "Résumé à venir.",
        "No publications are available yet.": "Aucune publication n'est encore disponible.",
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
        "☉ Tutorials": "☉ Tutorials",
        "Publications": "Publikationen",
        "இ Publications": "இ Publikationen",
        "Publication records with metadata and downloadable PDFs.": "Publikationsdatensätze mit Metadaten und herunterladbaren PDFs.",
        "Metadata records and downloadable PDFs for CoolBox publications.": "Metadatensätze und herunterladbare PDFs für CoolBox-Publikationen.",
        "Browse metadata and blank starter PDFs for publications.": "Metadaten und leere Starter-PDFs für Publikationen durchsuchen.",
        "Open publications": "Publikationen öffnen",
        "Publication details will be added later.": "Die Publikationsdetails werden später hinzugefügt.",
        "Abstract coming soon.": "Zusammenfassung folgt.",
        "No publications are available yet.": "Derzeit sind noch keine Publikationen verfügbar.",
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
        "☉ Tutorials": "☉ チュートリアル",
        "Publications": "出版物",
        "இ Publications": "இ 出版物",
        "Publication records with metadata and downloadable PDFs.": "メタデータとダウンロード可能な PDF を含む出版物レコードです。",
        "Metadata records and downloadable PDFs for CoolBox publications.": "CoolBox 出版物のメタデータ記録とダウンロード可能な PDF です。",
        "Browse metadata and blank starter PDFs for publications.": "出版物向けのメタデータと空のスターター PDF を参照できます。",
        "Open publications": "出版物を開く",
        "Publication details will be added later.": "出版物の詳細は後で追加されます。",
        "Abstract coming soon.": "要約は近日公開です。",
        "No publications are available yet.": "利用可能な出版物はまだありません。",
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
}

LANGUAGE_LABELS = {
    "en": "English",
    "es": "Español",
    "fr": "Français",
    "de": "Deutsch",
    "ja": "日本語",
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
        items.append(f'<a class="{css_class}" href="{href}">[{LANGUAGE_LABELS.get(locale, locale)}]</a>')

    block = (
        '<aside class="language-switcher" aria-label="Languages">'
        f'<div class="language-switcher-links">{" ".join(items)}</div>'
        '</aside>'
    )

    style_marker = "footer { margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; font-size: 0.95rem; }"
    if style_marker in html:
        html = html.replace(
            style_marker,
            style_marker
            + "\n    .language-switcher { position: fixed; right: 1.25rem; bottom: 1.25rem; background: rgba(255, 255, 255, 0.96); border: 1px solid #cbd5e1; border-radius: 14px; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.15); padding: 0.9rem 1rem; min-width: 12rem; z-index: 20; }"
            + "\n    .language-switcher-links { display: flex; gap: 0.5rem; flex-wrap: wrap; justify-content: flex-end; }"
            + "\n    .language-switcher a { font-weight: 600; white-space: nowrap; }"
            + "\n    .language-switcher a.active { color: #000000; }"
        )

    html = html.replace("</body>", "  " + block + "\n</body>", 1)

    index_path.write_text(html, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate localized copies of the built documentation site.")
    parser.add_argument("site_dir", help="Path to the generated site directory")
    parser.add_argument("--locales", nargs="*", default=list(DEFAULT_LOCALES), help="Locales to generate")
    parser.add_argument("--config", default="config/docs_translations.json", help="Optional JSON file with extra translation mappings")
    parser.add_argument("--catalog", default="config/docs_phrase_catalog.json", help="Optional JSON file with UUID-based phrase and paragraph translations")
    args = parser.parse_args()

    site_dir = Path(args.site_dir).resolve()
    if not site_dir.exists():
        raise FileNotFoundError(f"Site directory not found: {site_dir}")

    config_path = (site_dir.parent / args.config).resolve() if not Path(args.config).is_absolute() else Path(args.config)
    catalog_path = (site_dir.parent / args.catalog).resolve() if not Path(args.catalog).is_absolute() else Path(args.catalog)
    translations = merged_translations(config_path, catalog_path)
    locales = [locale for locale in args.locales if locale in translations]

    for locale in locales:
        translate_tree(site_dir, site_dir / locale, locale, translations[locale])

    inject_language_switcher(site_dir / "index.html", locales, "en")
    for locale in locales:
        inject_language_switcher(site_dir / locale / "index.html", locales, locale)
    print(f"Generated localized docs for: {', '.join(locales)}")


if __name__ == "__main__":
    main()