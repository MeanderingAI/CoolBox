#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path


DEFAULT_LOCALES = ("es", "fr", "de", "ja")

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
        "Meandering LLC © 2026": "Meandering LLC © 2026",
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
        "Meandering LLC © 2026": "Meandering LLC © 2026",
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
        "Meandering LLC © 2026": "Meandering LLC © 2026",
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
        "Meandering LLC © 2026": "Meandering LLC © 2026",
    },
}

LANGUAGE_LABELS = {
    "es": "Español",
    "fr": "Français",
    "de": "Deutsch",
    "ja": "日本語",
}


def load_custom_translations(config_path: Path) -> dict[str, dict[str, str]]:
    if not config_path.exists():
        return {}
    payload = json.loads(config_path.read_text(encoding="utf-8"))
    custom: dict[str, dict[str, str]] = {}
    for locale, mapping in payload.items():
        if isinstance(mapping, dict):
            custom[locale] = {str(key): str(value) for key, value in mapping.items()}
    return custom


def merged_translations(config_path: Path) -> dict[str, dict[str, str]]:
    translations = {locale: mapping.copy() for locale, mapping in BUILTIN_TRANSLATIONS.items()}
    for locale, mapping in load_custom_translations(config_path).items():
        translations.setdefault(locale, {}).update(mapping)
    return translations


def apply_replacements(text: str, replacements: dict[str, str], locale: str) -> str:
    for source, target in sorted(replacements.items(), key=lambda item: len(item[0]), reverse=True):
        text = text.replace(source, target)
    text = text.replace('<html lang="en">', f'<html lang="{locale}">')
    text = text.replace("<html lang=\"en\">", f"<html lang=\"{locale}\">")
    return text


def translate_tree(source_dir: Path, target_dir: Path, locale: str, replacements: dict[str, str]) -> None:
    if target_dir.exists():
        shutil.rmtree(target_dir)
    shutil.copytree(source_dir, target_dir, ignore=shutil.ignore_patterns(*DEFAULT_LOCALES, ".git", ".github"))

    for html_file in target_dir.rglob("*.html"):
        html = html_file.read_text(encoding="utf-8")
        html_file.write_text(apply_replacements(html, replacements, locale), encoding="utf-8")


def inject_language_switcher(index_path: Path, locales: list[str]) -> None:
    if not index_path.exists():
        return

    html = index_path.read_text(encoding="utf-8")
    if "language-switcher" in html:
        return

    items = "".join(
        f'<li><a href="{locale}/index.html">{LANGUAGE_LABELS.get(locale, locale)}</a></li>' for locale in locales
    )
    block = (
        '<section class="language-switcher">'
        '<h2 class="section-title">Language translations</h2>'
        '<p class="muted">Browse localized copies of the generated docs portal.</p>'
        f'<ul>{items}</ul>'
        '</section>'
        '<hr class="section-divider">'
    )

    style_marker = "footer { margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; font-size: 0.95rem; }"
    if style_marker in html:
        html = html.replace(style_marker, style_marker + "\n    .language-switcher ul { padding-left: 1.25rem; }\n    .language-switcher li { margin: 0.5rem 0; }")

    insert_after = "<hr class=\"section-divider\">"
    if insert_after in html:
        html = html.replace(insert_after, insert_after + "\n    " + block, 1)
    else:
        html = html.replace("</main>", block + "\n    </main>", 1)

    index_path.write_text(html, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate localized copies of the built documentation site.")
    parser.add_argument("site_dir", help="Path to the generated site directory")
    parser.add_argument("--locales", nargs="*", default=list(DEFAULT_LOCALES), help="Locales to generate")
    parser.add_argument("--config", default="config/docs_translations.json", help="Optional JSON file with extra translation mappings")
    args = parser.parse_args()

    site_dir = Path(args.site_dir).resolve()
    if not site_dir.exists():
        raise FileNotFoundError(f"Site directory not found: {site_dir}")

    translations = merged_translations((site_dir.parent / args.config).resolve() if not Path(args.config).is_absolute() else Path(args.config))
    locales = [locale for locale in args.locales if locale in translations]

    for locale in locales:
        translate_tree(site_dir, site_dir / locale, locale, translations[locale])

    inject_language_switcher(site_dir / "index.html", locales)
    print(f"Generated localized docs for: {', '.join(locales)}")


if __name__ == "__main__":
    main()