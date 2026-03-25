#!/usr/bin/env python3
from __future__ import annotations

import sys
import tkinter as tk
import webbrowser
from pathlib import Path
from tkinter import filedialog, messagebox, simpledialog, ttk

try:
    from tkinterdnd2 import DND_FILES, TkinterDnD  # type: ignore[import-not-found]
except ImportError:
    DND_FILES = None
    TkinterDnD = None

ROOT_DIR = Path(__file__).resolve().parents[1]
TUTORIALS_DIR = ROOT_DIR / "tutorials"
BACKAGES_DIR = ROOT_DIR / "_libraries" / "backages"
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from _scripts.build_tutorials import is_remote_url, normalize_library_reference, parse_tutorial, render_page

PREVIEW_WORK_DIR = ROOT_DIR / "build" / "tutorial-editor-preview"
PREVIEW_SITE_DIR = PREVIEW_WORK_DIR / "site"
DEFAULT_TEMPLATE = """@title1: New CoolBox Tutorial
@tags: tutorial
@libs: MISC/hash
@repo: https://github.com/example/repository

Write your introduction here.

@title2: Libraries Used
List the local library folders used in this tutorial here.

Example:
- <category>/<library>
- MISC/hash
- IO/http_server

@title2: Code Example
@code: cpp | MISC/hash, IO/http_server
// Add your example here
@endcode
"""

CODE_LANGUAGES = [
    "text",
    "python",
    "cpp",
    "c",
    "javascript",
    "typescript",
    "java",
    "go",
    "rust",
    "bash",
    "html",
    "css",
    "json",
    "sql",
    "yaml",
]

IMAGE_FILETYPES = [
    ("Image files", "*.png *.jpg *.jpeg *.gif *.bmp *.webp *.tif *.tiff"),
    ("All files", "*.*"),
]

VIDEO_FILETYPES = [
    ("Video files", "*.mp4 *.mov *.m4v *.avi *.mkv *.webm *.ogv"),
    ("All files", "*.*"),
]

IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".gif", ".bmp", ".webp", ".tif", ".tiff"}
VIDEO_EXTENSIONS = {".mp4", ".mov", ".m4v", ".avi", ".mkv", ".webm", ".ogv"}
TAG_CATALOG_FILENAMES = ("tags.txt", "tutorial_tags.txt")


def _detect_tkdnd_support() -> bool:
    if TkinterDnD is None:
        return False

    probe = None
    try:
        probe = TkinterDnD.Tk()
        probe.withdraw()
        probe.update_idletasks()
        return True
    except Exception:
        return False
    finally:
        if probe is not None:
            try:
                probe.destroy()
            except Exception:
                pass


TKDND_AVAILABLE = _detect_tkdnd_support()
BASE_TK = TkinterDnD.Tk if TKDND_AVAILABLE and TkinterDnD is not None else tk.Tk


def discover_local_library_references() -> list[str]:
    references: list[str] = []
    if not BACKAGES_DIR.exists():
        return references

    for cmake_file in sorted(BACKAGES_DIR.rglob("CMakeLists.txt")):
        library_dir = cmake_file.parent
        relative_dir = library_dir.relative_to(BACKAGES_DIR).as_posix()

        if relative_dir == ".":
            continue
        if any(part.startswith("__") or part.startswith("DISABLED") for part in library_dir.parts):
            continue

        references.append(normalize_library_reference(relative_dir))

    # Preserve order while removing duplicates.
    return list(dict.fromkeys(references))


class MediaSectionDialog(simpledialog.Dialog):
    def __init__(self, parent, media_kind: str, initial_dir: Path) -> None:
        self.media_kind = media_kind
        self.initial_dir = initial_dir
        self.result = None
        self.drop_available = TKDND_AVAILABLE and DND_FILES is not None
        super().__init__(parent, title=f"Insert {media_kind}")

    def body(self, master):
        label_text = "Alt text:" if self.media_kind == "image" else "Caption:"
        helper_text = "Add a local file or remote URL."

        ttk.Label(master, text="File path or URL:").grid(row=0, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.reference_var = tk.StringVar()
        ttk.Entry(master, textvariable=self.reference_var).grid(row=0, column=1, sticky="ew", pady=(0, 6))
        ttk.Button(master, text="Browse…", command=self.browse_file).grid(row=0, column=2, padx=(8, 0), pady=(0, 6))

        ttk.Label(master, text=label_text).grid(row=1, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.meta_var = tk.StringVar()
        ttk.Entry(master, textvariable=self.meta_var).grid(row=1, column=1, columnspan=2, sticky="ew", pady=(0, 6))

        ttk.Label(master, text=helper_text, foreground="#64748b").grid(row=2, column=1, columnspan=2, sticky="w")

        self.drop_target = tk.Label(
            master,
            text=(
                f"Drop a {self.media_kind} file here"
                if self.drop_available
                else "Drag-and-drop is unavailable on this system; use Browse or paste a path/URL instead"
            ),
            background="#eff6ff",
            foreground="#1d4ed8",
            relief="ridge",
            bd=1,
            padx=16,
            pady=20,
            justify="center",
        )
        self.drop_target.grid(row=3, column=0, columnspan=3, sticky="ew", pady=(10, 0))

        if self.drop_available:
            self.drop_target.drop_target_register(DND_FILES)
            self.drop_target.dnd_bind("<<Drop>>", self.on_drop)

        master.columnconfigure(1, weight=1)
        return master

    def browse_file(self) -> None:
        filetypes = IMAGE_FILETYPES if self.media_kind == "image" else VIDEO_FILETYPES
        path = filedialog.askopenfilename(
            title=f"Choose {self.media_kind}",
            initialdir=self.initial_dir,
            filetypes=filetypes,
            parent=self,
        )
        if path:
            self.reference_var.set(path)

    def on_drop(self, event) -> str:
        paths = [item for item in self.tk.splitlist(event.data) if item]
        if paths:
            self.reference_var.set(paths[0])
        return "break"

    def apply(self) -> None:
        self.result = {
            "reference": self.reference_var.get().strip(),
            "meta": self.meta_var.get().strip(),
        }


class CodeSectionDialog(simpledialog.Dialog):
    def __init__(self, parent, available_libraries: list[str] | None = None, title: str | None = None):
        self.available_libraries = available_libraries or []
        super().__init__(parent, title=title)

    def body(self, master):
        self.result = None

        ttk.Label(master, text="Language:").grid(row=0, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.language_var = tk.StringVar(value="python")
        self.language_combo = ttk.Combobox(master, textvariable=self.language_var, values=CODE_LANGUAGES, state="normal", width=18)
        self.language_combo.grid(row=0, column=1, sticky="ew", pady=(0, 6))

        ttk.Label(master, text="Libraries used:").grid(row=1, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.libs_var = tk.StringVar()
        ttk.Entry(master, textvariable=self.libs_var).grid(row=1, column=1, sticky="ew", pady=(0, 6))

        ttk.Label(master, text="Add local library:").grid(row=2, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.library_picker_var = tk.StringVar()
        self.library_picker = ttk.Combobox(
            master,
            textvariable=self.library_picker_var,
            values=self.available_libraries,
            state="normal",
            width=40,
        )
        self.library_picker.grid(row=2, column=1, sticky="ew", pady=(0, 6))
        ttk.Button(master, text="Add", command=self.add_selected_library).grid(row=2, column=2, padx=(8, 0), pady=(0, 6))

        ttk.Label(master, text="Code:").grid(row=3, column=0, sticky="nw", padx=(0, 8))
        self.code_text = tk.Text(master, width=70, height=14, wrap="none", font=("Menlo", 11))
        self.code_text.grid(row=3, column=1, sticky="nsew")

        scroll = ttk.Scrollbar(master, orient="vertical", command=self.code_text.yview)
        scroll.grid(row=3, column=2, sticky="ns")
        self.code_text.configure(yscrollcommand=scroll.set)

        helper = ttk.Label(
            master,
            text="Comma-separate local library folders under backages/, for example: MISC/hash, IO/http_server",
            foreground="#64748b",
            wraplength=540,
            justify="left",
        )
        helper.grid(row=4, column=1, sticky="w", pady=(6, 0))

        master.columnconfigure(1, weight=1)
        master.rowconfigure(3, weight=1)
        return self.language_combo

    def add_selected_library(self) -> None:
        selected = self.library_picker_var.get().strip()
        if not selected:
            return

        existing = [item.strip() for item in self.libs_var.get().split(",") if item.strip()]
        if selected not in existing:
            existing.append(selected)
            self.libs_var.set(", ".join(existing))
        self.library_picker_var.set("")

    def apply(self) -> None:
        language = self.language_var.get().strip() or "text"
        libraries = ", ".join(
            normalize_library_reference(item)
            for item in self.libs_var.get().split(",")
            if item.strip()
        )
        code = self.code_text.get("1.0", tk.END).rstrip()
        self.result = {
            "language": language,
            "libraries": libraries,
            "code": code,
        }


class TutorialEditor(BASE_TK):
    def __init__(self) -> None:
        super().__init__()
        self.title("CoolBox Tutorial Editor")
        self.geometry("1180x760")
        self.minsize(980, 620)
        self.current_file: Path | None = None
        self.is_dirty = False
        self.preview_job: str | None = None
        self.preview_html_path: Path | None = None
        self.preview_images: list[tk.PhotoImage] = []
        self.drag_drop_enabled = TKDND_AVAILABLE and DND_FILES is not None
        self.available_library_refs = discover_local_library_references()
        self.available_tags: list[str] = []
        self.directory_files: list[Path] = []

        self._build_layout()
        self.load_template()
        self.reload_tag_catalog()
        self.sync_tags_from_source()
        self.refresh_directory_file_list()
        self.refresh_preview()

    def _build_layout(self) -> None:
        self.columnconfigure(0, weight=1)
        self.rowconfigure(1, weight=1)

        toolbar = ttk.Frame(self, padding=(12, 10))
        toolbar.grid(row=0, column=0, sticky="ew")
        for idx in range(18):
            toolbar.columnconfigure(idx, weight=0)
        toolbar.columnconfigure(18, weight=1)

        ttk.Button(toolbar, text="New", command=self.new_file).grid(row=0, column=0, padx=4)
        ttk.Button(toolbar, text="Open", command=self.open_file).grid(row=0, column=1, padx=4)
        ttk.Button(toolbar, text="Save", command=self.save_file).grid(row=0, column=2, padx=4)
        ttk.Button(toolbar, text="Save As", command=self.save_file_as).grid(row=0, column=3, padx=4)
        ttk.Button(toolbar, text="Save Draft", command=self.save_draft).grid(row=0, column=4, padx=4)
        ttk.Separator(toolbar, orient="vertical").grid(row=0, column=5, sticky="ns", padx=8)
        ttk.Label(toolbar, text="Directory files:").grid(row=0, column=6, padx=(0, 4))
        self.directory_file_var = tk.StringVar()
        self.directory_file_picker = ttk.Combobox(toolbar, textvariable=self.directory_file_var, state="readonly", width=28)
        self.directory_file_picker.grid(row=0, column=7, padx=4)
        self.directory_file_picker.bind("<<ComboboxSelected>>", self.on_directory_file_selected)
        ttk.Button(toolbar, text="Refresh Files", command=self.refresh_directory_file_list).grid(row=0, column=8, padx=4)
        ttk.Separator(toolbar, orient="vertical").grid(row=0, column=9, sticky="ns", padx=8)
        ttk.Button(toolbar, text="Title", command=lambda: self.insert_title(1)).grid(row=0, column=10, padx=4)
        ttk.Button(toolbar, text="Subtitle", command=lambda: self.insert_title(2)).grid(row=0, column=11, padx=4)
        ttk.Button(toolbar, text="Image", command=self.insert_image).grid(row=0, column=12, padx=4)
        ttk.Button(toolbar, text="Video", command=self.insert_video).grid(row=0, column=13, padx=4)
        ttk.Button(toolbar, text="Link", command=self.insert_link).grid(row=0, column=14, padx=4)
        ttk.Button(toolbar, text="Tags", command=self.insert_tags).grid(row=0, column=15, padx=4)
        ttk.Button(toolbar, text="Paragraph", command=self.insert_paragraph).grid(row=0, column=16, padx=4)
        ttk.Button(toolbar, text="Code", command=self.insert_code_section).grid(row=0, column=17, padx=4)

        self.status_var = tk.StringVar(value="Ready")
        ttk.Label(toolbar, textvariable=self.status_var, anchor="e").grid(row=0, column=18, sticky="ew", padx=(12, 0))

        content = ttk.Panedwindow(self, orient=tk.HORIZONTAL)
        content.grid(row=1, column=0, sticky="nsew", padx=12, pady=(0, 12))

        editor_frame = ttk.Frame(content, padding=(0, 0, 8, 0))
        editor_frame.columnconfigure(0, weight=1)
        editor_frame.rowconfigure(3, weight=1)
        ttk.Label(editor_frame, text=".tut Source", font=("Arial", 13, "bold")).grid(row=0, column=0, sticky="w", pady=(0, 8))

        media_drop_text = (
            "Drop image or video files here to insert media directives, or use the Image/Video buttons."
            if self.drag_drop_enabled
            else "Use the Image/Video buttons to choose local files or URLs. Drag-and-drop is currently unavailable on this system."
        )
        self.media_drop_label = tk.Label(
            editor_frame,
            text=media_drop_text,
            background="#eff6ff",
            foreground="#1d4ed8",
            relief="ridge",
            bd=1,
            padx=14,
            pady=10,
            justify="left",
            anchor="w",
        )
        self.media_drop_label.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(0, 8))

        if self.drag_drop_enabled:
            self.media_drop_label.drop_target_register(DND_FILES)
            self.media_drop_label.dnd_bind("<<Drop>>", self.on_media_drop)

        tags_panel = ttk.LabelFrame(editor_frame, text="Tags", padding=(10, 8))
        tags_panel.grid(row=2, column=0, columnspan=2, sticky="ew", pady=(0, 8))
        tags_panel.columnconfigure(1, weight=1)

        ttk.Label(tags_panel, text="Current tags:").grid(row=0, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.tags_var = tk.StringVar()
        ttk.Entry(tags_panel, textvariable=self.tags_var).grid(row=0, column=1, sticky="ew", pady=(0, 6))
        ttk.Button(tags_panel, text="Apply Tags", command=self.apply_tags_from_entry).grid(row=0, column=2, padx=(8, 0), pady=(0, 6))

        ttk.Label(tags_panel, text="Current libs:").grid(row=1, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.libs_var = tk.StringVar()
        ttk.Entry(tags_panel, textvariable=self.libs_var).grid(row=1, column=1, sticky="ew", pady=(0, 6))
        ttk.Button(tags_panel, text="Apply Libs", command=self.apply_libs_from_entry).grid(row=1, column=2, padx=(8, 0), pady=(0, 6))

        ttk.Label(tags_panel, text="Repository:").grid(row=2, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.repo_var = tk.StringVar()
        ttk.Entry(tags_panel, textvariable=self.repo_var).grid(row=2, column=1, sticky="ew", pady=(0, 6))
        ttk.Button(tags_panel, text="Apply Repo", command=self.apply_repo_from_entry).grid(row=2, column=2, padx=(8, 0), pady=(0, 6))

        ttk.Label(tags_panel, text="Suggested tag:").grid(row=3, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.tag_picker_var = tk.StringVar()
        self.tag_picker = ttk.Combobox(tags_panel, textvariable=self.tag_picker_var, values=(), state="readonly")
        self.tag_picker.grid(row=3, column=1, sticky="ew", pady=(0, 6))
        self.add_tag_button = ttk.Button(tags_panel, text="Add tag", command=self.add_selected_tag)
        self.add_tag_button.grid(row=3, column=2, padx=(8, 0), pady=(0, 6))

        ttk.Label(tags_panel, text="Library tag:").grid(row=4, column=0, sticky="w", padx=(0, 8), pady=(0, 6))
        self.library_tag_var = tk.StringVar()
        self.library_tag_picker = ttk.Combobox(tags_panel, textvariable=self.library_tag_var, values=self.available_library_refs, state="readonly")
        self.library_tag_picker.grid(row=4, column=1, sticky="ew", pady=(0, 6))
        if self.available_library_refs:
            self.library_tag_picker.current(0)
        self.add_library_tag_button = ttk.Button(tags_panel, text="Add lib", command=self.add_selected_library_tag)
        self.add_library_tag_button.grid(row=4, column=2, padx=(8, 0), pady=(0, 6))

        ttk.Button(tags_panel, text="Reload Tag File", command=self.reload_tag_catalog).grid(row=5, column=2, padx=(8, 0), sticky="e")

        self.tag_picker_var.trace_add("write", self.on_tag_picker_changed)
        self.library_tag_var.trace_add("write", self.on_library_tag_picker_changed)
        self.tag_picker.bind("<<ComboboxSelected>>", self.on_tag_picker_changed)
        self.library_tag_picker.bind("<<ComboboxSelected>>", self.on_library_tag_picker_changed)
        self.on_library_tag_picker_changed()

        self.text = tk.Text(editor_frame, wrap="word", undo=True, font=("Menlo", 12))
        self.text.grid(row=3, column=0, sticky="nsew")
        editor_scroll = ttk.Scrollbar(editor_frame, orient="vertical", command=self.text.yview)
        editor_scroll.grid(row=3, column=1, sticky="ns")
        self.text.configure(yscrollcommand=editor_scroll.set)
        self.text.bind("<<Modified>>", self.on_text_modified)
        if self.drag_drop_enabled:
            self.text.drop_target_register(DND_FILES)
            self.text.dnd_bind("<<Drop>>", self.on_media_drop)

        preview_frame = ttk.Frame(content, padding=(8, 0, 0, 0))
        preview_frame.columnconfigure(0, weight=1)
        preview_frame.rowconfigure(1, weight=1)
        ttk.Label(preview_frame, text="Preview Workspace", font=("Arial", 13, "bold")).grid(row=0, column=0, sticky="w", pady=(0, 8))

        notebook = ttk.Notebook(preview_frame)
        notebook.grid(row=1, column=0, columnspan=2, sticky="nsew")

        guide_tab = ttk.Frame(notebook, padding=8)
        guide_tab.columnconfigure(0, weight=1)
        guide_tab.rowconfigure(0, weight=1)
        guide = tk.Text(
            guide_tab,
            wrap="word",
            state="normal",
            font=("Arial", 11),
            background="#f8fafc",
            foreground="#0f172a",
            relief="flat",
            insertwidth=0,
            takefocus=0,
        )
        guide.grid(row=0, column=0, sticky="nsew")
        guide_scroll = ttk.Scrollbar(guide_tab, orient="vertical", command=guide.yview)
        guide_scroll.grid(row=0, column=1, sticky="ns")
        guide.configure(yscrollcommand=guide_scroll.set)
        guide.insert(
            "1.0",
            "Supported tutorial syntax\n\n"
            "@title1: Main title\n"
            "@title2: Section title\n"
            "@title3: Smaller heading\n\n"
            "@tags: docs, intro, tutorial\n"
            "@libs: MISC/hash, IO/http_server\n\n"
            "@repo: https://github.com/example/repository\n\n"
            "@image: assets/example.png | Optional alt text\n"
            "@video: https://example.com/video.mp4 | Optional caption\n"
            "@link: https://example.com | Link label\n\n"
            "@code: cpp | MISC/hash, IO/http_server\n"
            "#include \"password_hash.hpp\"\n"
            "// Example code goes here\n"
            "@endcode\n\n"
            "Plain text lines become paragraphs. Blank lines separate paragraphs.\n\n"
            "Tips\n"
            "• Save tutorials into the top-level tutorials/ folder.\n"
            "• Put reusable tags in tags.txt or tutorial_tags.txt beside the tutorial file to populate the tag picker.\n"
            "• Keep local media next to the tutorial file or in tutorials/assets/.\n"
            "• Use the Image/Video buttons to browse for files, or drag media files into the editor.\n"
            "• The editor keeps the top metadata ordered as @title, then @tags, then @libs, then @repo.\n"
            "• For code sections, list local libraries as folder references relative to backages/, such as MISC/hash or IO/http_server.\n"
            "• The tutorials pipeline will convert each .tut file into static HTML.\n",
        )
        guide.bind("<Key>", lambda _event: "break")
        guide.bind("<<Paste>>", lambda _event: "break")
        guide.bind("<<Cut>>", lambda _event: "break")
        guide.bind("<<Clear>>", lambda _event: "break")

        preview_tab = ttk.Frame(notebook, padding=8)
        preview_tab.columnconfigure(0, weight=1)
        preview_tab.rowconfigure(1, weight=1)

        preview_toolbar = ttk.Frame(preview_tab)
        preview_toolbar.grid(row=0, column=0, sticky="ew", pady=(0, 8))
        preview_toolbar.columnconfigure(2, weight=1)
        ttk.Button(preview_toolbar, text="Refresh Preview", command=self.refresh_preview).grid(row=0, column=0, padx=(0, 8))
        ttk.Button(preview_toolbar, text="Open in Browser", command=self.open_preview_in_browser).grid(row=0, column=1)
        self.preview_status_var = tk.StringVar(value="Preview ready")
        ttk.Label(preview_toolbar, textvariable=self.preview_status_var, anchor="e").grid(row=0, column=2, sticky="ew")

        self.preview_canvas = tk.Canvas(preview_tab, background="#f8fafc", highlightthickness=0)
        self.preview_canvas.grid(row=1, column=0, sticky="nsew")
        preview_scroll = ttk.Scrollbar(preview_tab, orient="vertical", command=self.preview_canvas.yview)
        preview_scroll.grid(row=1, column=1, sticky="ns")
        self.preview_canvas.configure(yscrollcommand=preview_scroll.set)
        self.preview_content = ttk.Frame(self.preview_canvas, padding=16)
        self.preview_window = self.preview_canvas.create_window((0, 0), window=self.preview_content, anchor="nw")
        self.preview_content.bind("<Configure>", self.on_preview_content_configure)
        self.preview_canvas.bind("<Configure>", self.on_preview_canvas_configure)

        html_tab = ttk.Frame(notebook, padding=8)
        html_tab.columnconfigure(0, weight=1)
        html_tab.rowconfigure(0, weight=1)
        self.html_preview = tk.Text(html_tab, wrap="none", state="disabled", font=("Menlo", 10), background="#0f172a", foreground="#e2e8f0")
        self.html_preview.grid(row=0, column=0, sticky="nsew")
        html_scroll = ttk.Scrollbar(html_tab, orient="vertical", command=self.html_preview.yview)
        html_scroll.grid(row=0, column=1, sticky="ns")
        self.html_preview.configure(yscrollcommand=html_scroll.set)

        notebook.add(guide_tab, text="Guide")
        notebook.add(preview_tab, text="Preview")
        notebook.add(html_tab, text="HTML")

        content.add(editor_frame, weight=3)
        content.add(preview_frame, weight=2)

    def load_template(self) -> None:
        self.text.delete("1.0", tk.END)
        self.text.insert("1.0", DEFAULT_TEMPLATE)
        self.text.edit_modified(False)
        self.is_dirty = False
        self.sync_tags_from_source()

    def get_current_directory(self) -> Path:
        return self.current_file.parent if self.current_file else TUTORIALS_DIR

    def get_draft_path(self) -> Path:
        base_dir = self.get_current_directory()
        if self.current_file is not None:
            return self.current_file.with_name(f"{self.current_file.stem}.draft{self.current_file.suffix}")
        return base_dir / "untitled.draft.tut"

    def refresh_directory_file_list(self) -> None:
        directory = self.get_current_directory()
        directory.mkdir(parents=True, exist_ok=True)
        files = sorted(
            [path for path in directory.iterdir() if path.is_file()],
            key=lambda path: path.name.lower(),
        )
        self.directory_files = files
        display_names = [path.name for path in files]
        if hasattr(self, "directory_file_picker"):
            self.directory_file_picker.configure(values=display_names)
            if self.current_file is not None and self.current_file.name in display_names:
                self.directory_file_var.set(self.current_file.name)
            elif display_names:
                self.directory_file_var.set(display_names[0])
            else:
                self.directory_file_var.set("")

    def load_file_into_editor(self, path: Path, status_message: str | None = None) -> None:
        self.text.delete("1.0", tk.END)
        self.text.insert("1.0", path.read_text(encoding="utf-8"))
        self.current_file = path
        self.text.edit_modified(False)
        self.is_dirty = False
        self.reload_tag_catalog()
        self.sync_tags_from_source()
        self.refresh_directory_file_list()
        self.status_var.set(status_message or f"Opened {path.name}")
        self.refresh_preview()

    def save_current_to_path(self, path: Path, status_message: str) -> bool:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(self.text.get("1.0", tk.END).rstrip() + "\n", encoding="utf-8")
        self.text.edit_modified(False)
        self.is_dirty = False
        self.refresh_directory_file_list()
        self.status_var.set(status_message)
        self.refresh_preview()
        return True

    def ask_discard_changes(self) -> bool:
        if not self.is_dirty:
            return True
        result = messagebox.askyesnocancel("Unsaved changes", "Save the current tutorial before continuing?")
        if result is None:
            return False
        if result:
            return self.save_file()
        return True

    def new_file(self) -> None:
        if not self.ask_discard_changes():
            return
        self.current_file = None
        self.load_template()
        self.reload_tag_catalog()
        self.refresh_directory_file_list()
        self.status_var.set("New tutorial")
        self.refresh_preview()

    def open_file(self) -> None:
        if not self.ask_discard_changes():
            return
        file_path = filedialog.askopenfilename(
            title="Open tutorial",
            initialdir=TUTORIALS_DIR,
            filetypes=[("Tutorial files", "*.tut"), ("All files", "*.*")],
        )
        if not file_path:
            return
        path = Path(file_path)
        self.load_file_into_editor(path)

    def save_file(self) -> bool:
        if self.current_file is None:
            return self.save_file_as()
        return self.save_current_to_path(self.current_file, f"Saved {self.current_file.name}")

    def save_file_as(self) -> bool:
        TUTORIALS_DIR.mkdir(parents=True, exist_ok=True)
        file_path = filedialog.asksaveasfilename(
            title="Save tutorial as",
            initialdir=TUTORIALS_DIR,
            defaultextension=".tut",
            filetypes=[("Tutorial files", "*.tut"), ("All files", "*.*")],
        )
        if not file_path:
            return False
        self.current_file = Path(file_path)
        self.reload_tag_catalog()
        return self.save_file()

    def save_draft(self) -> bool:
        draft_path = self.get_draft_path()
        return self.save_current_to_path(draft_path, f"Saved draft {draft_path.name}")

    def on_directory_file_selected(self, _event=None) -> None:
        selection = self.directory_file_picker.get().strip()
        if not selection:
            return
        selected_path = self.get_current_directory() / selection
        if not selected_path.exists() or not selected_path.is_file():
            return
        if self.current_file is not None and selected_path.resolve() == self.current_file.resolve():
            return
        if not self.ask_discard_changes():
            self.refresh_directory_file_list()
            return
        self.load_file_into_editor(selected_path)

    def insert_text(self, text: str) -> None:
        self.text.insert(tk.INSERT, text)
        self.text.focus_set()

    def insert_title(self, level: int) -> None:
        title = simpledialog.askstring("Insert title", f"Enter title text for @title{level}:", parent=self)
        if title:
            self.insert_text(f"@title{level}: {title}\n")
            self.status_var.set(f"Inserted @title{level}")

    def insert_tags(self) -> None:
        tags = simpledialog.askstring("Insert tags", "Comma-separated tags:", parent=self)
        if tags:
            self.set_source_tags([item.strip() for item in tags.split(",") if item.strip()])
            self.status_var.set("Updated tags")

    def get_tag_catalog_path(self) -> Path | None:
        base_dir = self.current_file.parent if self.current_file else TUTORIALS_DIR
        for filename in TAG_CATALOG_FILENAMES:
            candidate = base_dir / filename
            if candidate.exists():
                return candidate
        return None

    def load_available_tags(self) -> list[str]:
        catalog_path = self.get_tag_catalog_path()
        if catalog_path is None:
            return []

        tags: list[str] = []
        for line in catalog_path.read_text(encoding="utf-8").splitlines():
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue
            for item in stripped.split(","):
                tag = item.strip()
                if tag:
                    tags.append(tag)

        return list(dict.fromkeys(tags))

    def reload_tag_catalog(self) -> None:
        self.available_tags = self.load_available_tags()
        if hasattr(self, "tag_picker"):
            self.tag_picker.configure(values=self.available_tags)
        if not self.available_tags:
            self.tag_picker_var.set("")
            self.add_tag_button.configure(text="Add tag")

    def get_source_tags(self) -> list[str]:
        for line in self.text.get("1.0", tk.END).splitlines():
            if line.startswith("@tags:"):
                return [item.strip() for item in line.split(":", 1)[1].split(",") if item.strip()]
        return []

    def get_source_libs(self) -> list[str]:
        for line in self.text.get("1.0", tk.END).splitlines():
            if line.startswith("@libs:"):
                return [normalize_library_reference(item) for item in line.split(":", 1)[1].split(",") if item.strip()]
        return []

    def get_source_repo(self) -> str:
        for line in self.text.get("1.0", tk.END).splitlines():
            if line.startswith("@repo:"):
                return line.split(":", 1)[1].strip()
        return ""

    def sync_tags_from_source(self) -> None:
        if hasattr(self, "tags_var"):
            self.tags_var.set(", ".join(self.get_source_tags()))
        if hasattr(self, "libs_var"):
            self.libs_var.set(", ".join(self.get_source_libs()))
        if hasattr(self, "repo_var"):
            self.repo_var.set(self.get_source_repo())

    def set_source_metadata(self, tags: list[str] | None = None, libs: list[str] | None = None, repo: str | None = None) -> None:
        unique_tags = list(dict.fromkeys(tag.strip() for tag in (tags if tags is not None else self.get_source_tags()) if tag.strip()))
        unique_libs = list(dict.fromkeys(normalize_library_reference(lib) for lib in (libs if libs is not None else self.get_source_libs()) if lib.strip()))
        repo_value = (repo if repo is not None else self.get_source_repo()).strip()
        content_lines = self.text.get("1.0", tk.END).splitlines()
        filtered_lines = [
            line
            for line in content_lines
            if not line.startswith("@tags:") and not line.startswith("@libs:") and not line.startswith("@repo:")
        ]
        insert_at = 1 if filtered_lines and filtered_lines[0].startswith("@title") else 0
        metadata_lines: list[str] = []
        if unique_tags:
            metadata_lines.append(f"@tags: {', '.join(unique_tags)}")
        if unique_libs:
            metadata_lines.append(f"@libs: {', '.join(unique_libs)}")
        if repo_value:
            metadata_lines.append(f"@repo: {repo_value}")
        for offset, line in enumerate(metadata_lines):
            filtered_lines.insert(insert_at + offset, line)

        updated = "\n".join(filtered_lines).rstrip() + "\n"
        self.text.delete("1.0", tk.END)
        self.text.insert("1.0", updated)
        self.text.edit_modified(False)
        self.is_dirty = True
        self.sync_tags_from_source()
        self.schedule_preview_refresh()

    def set_source_tags(self, tags: list[str]) -> None:
        self.set_source_metadata(tags=tags)

    def set_source_libs(self, libs: list[str]) -> None:
        self.set_source_metadata(libs=libs)

    def set_source_repo(self, repo: str) -> None:
        self.set_source_metadata(repo=repo)

    def apply_tags_from_entry(self) -> None:
        tags = [item.strip() for item in self.tags_var.get().split(",") if item.strip()]
        self.set_source_tags(tags)
        self.status_var.set("Updated tags")

    def apply_libs_from_entry(self) -> None:
        libs = [normalize_library_reference(item) for item in self.libs_var.get().split(",") if item.strip()]
        self.set_source_libs(libs)
        self.status_var.set("Updated libs")

    def apply_repo_from_entry(self) -> None:
        self.set_source_repo(self.repo_var.get().strip())
        self.status_var.set("Updated repo link")

    def add_tag(self, tag: str) -> None:
        if not tag.strip():
            return
        tags = self.get_source_tags()
        tags.append(tag.strip())
        self.set_source_tags(tags)
        self.status_var.set(f"Added tag {tag.strip()}")

    def add_selected_tag(self) -> None:
        tag = self.tag_picker.get().strip()
        if tag:
            self.add_tag(tag)

    def add_selected_library_tag(self) -> None:
        library = normalize_library_reference(self.library_tag_picker.get())
        if library:
            self.add_lib(library)
        else:
            self.status_var.set("Choose a library tag first")

    def add_lib(self, library: str) -> None:
        if not library.strip():
            return
        libs = self.get_source_libs()
        libs.append(normalize_library_reference(library))
        self.set_source_libs(libs)
        self.status_var.set(f"Added lib {normalize_library_reference(library)}")

    def on_tag_picker_changed(self, *_args) -> None:
        tag = self.tag_picker.get().strip()
        label = f"Add tag: {tag}" if tag else "Add tag"
        self.add_tag_button.configure(text=label)

    def on_library_tag_picker_changed(self, *_args) -> None:
        library = normalize_library_reference(self.library_tag_picker.get())
        label = f"Add lib: {library}" if library else "Add lib"
        self.add_library_tag_button.configure(text=label)

    def get_media_base_dir(self) -> Path:
        return self.current_file.parent if self.current_file else TUTORIALS_DIR

    def format_media_reference(self, reference: str) -> str:
        if not reference or is_remote_url(reference):
            return reference

        raw_path = Path(reference).expanduser()
        path = raw_path.resolve() if raw_path.is_absolute() else raw_path
        base_dir = self.get_media_base_dir().resolve()

        try:
            return path.relative_to(base_dir).as_posix()
        except ValueError:
            return path.as_posix()

    def infer_media_kind(self, path: Path) -> str | None:
        suffix = path.suffix.lower()
        if suffix in IMAGE_EXTENSIONS:
            return "image"
        if suffix in VIDEO_EXTENSIONS:
            return "video"
        return None

    def insert_media_directive(self, media_kind: str, reference: str, meta: str = "") -> bool:
        reference = reference.strip()
        if not reference:
            return False

        payload = f"@{media_kind}: {self.format_media_reference(reference)}"
        if meta.strip():
            payload += f" | {meta.strip()}"
        self.insert_text(payload + "\n")
        self.status_var.set(f"Inserted {media_kind}")
        return True

    def prompt_media_section(self, media_kind: str) -> None:
        dialog = MediaSectionDialog(self, media_kind=media_kind, initial_dir=self.get_media_base_dir())
        if not dialog.result:
            return

        reference = str(dialog.result["reference"]).strip()
        meta = str(dialog.result["meta"]).strip()
        if self.insert_media_directive(media_kind, reference, meta):
            self.text.focus_set()

    def insert_image(self) -> None:
        self.prompt_media_section("image")

    def insert_video(self) -> None:
        self.prompt_media_section("video")

    def insert_link(self) -> None:
        href = simpledialog.askstring("Insert link", "URL:", parent=self)
        if not href:
            return
        label = simpledialog.askstring("Insert link", "Label:", parent=self) or href
        self.insert_text(f"@link: {href} | {label}\n")
        self.status_var.set("Inserted link")

    def insert_paragraph(self) -> None:
        body = simpledialog.askstring("Insert paragraph", "Paragraph text:", parent=self)
        if body:
            self.insert_text(body + "\n\n")
            self.status_var.set("Inserted paragraph")

    def insert_code_section(self) -> None:
        dialog = CodeSectionDialog(self, available_libraries=self.available_library_refs, title="Insert code section")
        if not dialog.result:
            return

        language = str(dialog.result["language"]).strip() or "text"
        libraries = str(dialog.result["libraries"]).strip()
        code = str(dialog.result["code"]).rstrip()
        header = f"@code: {language}" + (f" | {libraries}" if libraries else "")
        block = header + "\n"
        if code:
            block += code + "\n"
        block += "@endcode\n\n"
        self.insert_text(block)
        self.status_var.set(f"Inserted {language} code block")

    def on_media_drop(self, event) -> str:
        inserted = 0
        unsupported: list[str] = []

        for item in self.tk.splitlist(event.data):
            reference = str(item).strip()
            if not reference:
                continue

            media_kind = self.infer_media_kind(Path(reference))
            if media_kind is None:
                unsupported.append(Path(reference).name or reference)
                continue

            if self.insert_media_directive(media_kind, reference):
                inserted += 1

        if unsupported:
            self.status_var.set("Unsupported dropped file type")
            messagebox.showinfo(
                "Unsupported media",
                "Only image and video files can be dropped into the tutorial editor.\n\n"
                + "Ignored: "
                + ", ".join(unsupported),
                parent=self,
            )
        elif inserted:
            self.status_var.set(f"Inserted {inserted} media item{'s' if inserted != 1 else ''}")

        return "break"

    def on_text_modified(self, _event: tk.Event[tk.Text]) -> None:
        if self.text.edit_modified():
            self.is_dirty = True
            self.text.edit_modified(False)
            self.sync_tags_from_source()
            self.schedule_preview_refresh()

    def schedule_preview_refresh(self) -> None:
        if self.preview_job is not None:
            self.after_cancel(self.preview_job)
        self.preview_job = self.after(350, self.refresh_preview)

    def get_preview_source_path(self) -> Path:
        base_dir = self.current_file.parent if self.current_file else TUTORIALS_DIR
        name = self.current_file.name if self.current_file else "__preview__.tut"
        return base_dir / f".{name}.preview"

    def write_preview_source(self) -> Path:
        preview_source = self.get_preview_source_path()
        preview_source.parent.mkdir(parents=True, exist_ok=True)
        preview_source.write_text(self.text.get("1.0", tk.END).rstrip() + "\n", encoding="utf-8")
        return preview_source

    def refresh_preview(self) -> None:
        self.preview_job = None
        try:
            preview_source = self.write_preview_source()
            page = parse_tutorial(preview_source)
            PREVIEW_SITE_DIR.mkdir(parents=True, exist_ok=True)
            render_page(page, PREVIEW_SITE_DIR)
            self.preview_html_path = page.output_path
            self.render_preview_widgets(page)
            self.load_html_preview(page)
            self.preview_status_var.set(f"Preview updated: {page.output_path.name if page.output_path else page.slug}")
        except Exception as exc:
            self.preview_status_var.set("Preview error")
            self.show_preview_error(str(exc))

    def load_html_preview(self, page) -> None:
        html_path = page.output_path
        html_text = html_path.read_text(encoding="utf-8") if html_path and html_path.exists() else ""
        self.html_preview.configure(state="normal")
        self.html_preview.delete("1.0", tk.END)
        self.html_preview.insert("1.0", html_text)
        self.html_preview.configure(state="disabled")

    def render_preview_widgets(self, page) -> None:
        for child in self.preview_content.winfo_children():
            child.destroy()
        self.preview_images.clear()

        row = 0
        ttk.Label(self.preview_content, text=page.title, font=("Arial", 22, "bold"), wraplength=420).grid(row=row, column=0, sticky="w")
        row += 1

        if page.tags:
            tags_frame = ttk.Frame(self.preview_content)
            tags_frame.grid(row=row, column=0, sticky="w", pady=(8, 12))
            for idx, tag in enumerate(page.tags):
                ttk.Label(tags_frame, text=tag, background="#dbeafe", foreground="#1d4ed8", padding=(8, 4)).grid(row=0, column=idx, padx=(0, 6))
            row += 1

        if getattr(page, "libs", None):
            libs_frame = ttk.Frame(self.preview_content)
            libs_frame.grid(row=row, column=0, sticky="w", pady=(0, 12))
            ttk.Label(libs_frame, text="Libraries:", foreground="#166534").grid(row=0, column=0, padx=(0, 8))
            for idx, lib in enumerate(page.libs, start=1):
                ttk.Label(libs_frame, text=lib, background="#dcfce7", foreground="#166534", padding=(8, 4)).grid(row=0, column=idx, padx=(0, 6))
            row += 1

        if getattr(page, "repo", ""):
            repo_frame = ttk.Frame(self.preview_content)
            repo_frame.grid(row=row, column=0, sticky="w", pady=(0, 12))
            ttk.Label(repo_frame, text="Repository:", foreground="#2563eb").grid(row=0, column=0, padx=(0, 8))
            repo_link = ttk.Label(repo_frame, text=page.repo, foreground="#2563eb", cursor="hand2", wraplength=380, justify="left")
            repo_link.grid(row=0, column=1, sticky="w")
            repo_link.bind("<Button-1>", lambda _e, url=page.repo: webbrowser.open(url))
            row += 1

        first_title_skipped = False
        for block in page.blocks:
            if block.kind == "title" and not first_title_skipped and int(block.data["level"]) == 1 and str(block.data["text"]) == page.title:
                first_title_skipped = True
                continue

            widget = self.build_preview_widget(block)
            if widget is None:
                continue
            widget.grid(row=row, column=0, sticky="ew", pady=(0, 12))
            row += 1

        self.preview_content.columnconfigure(0, weight=1)

    def build_preview_widget(self, block):
        if block.kind == "title":
            level = int(block.data["level"])
            size = {1: 22, 2: 18, 3: 16, 4: 14, 5: 13, 6: 12}.get(level, 12)
            return ttk.Label(self.preview_content, text=str(block.data["text"]), font=("Arial", size, "bold"), wraplength=420, justify="left")

        if block.kind == "paragraph":
            return ttk.Label(self.preview_content, text=str(block.data["text"]), wraplength=420, justify="left")

        if block.kind == "link":
            frame = ttk.Frame(self.preview_content)
            link = ttk.Label(frame, text=str(block.data["label"]), foreground="#2563eb", cursor="hand2")
            link.grid(row=0, column=0, sticky="w")
            link.bind("<Button-1>", lambda _e, url=str(block.data["href"]): webbrowser.open(url))
            ttk.Label(frame, text=str(block.data["href"]), foreground="#64748b", wraplength=420, justify="left").grid(row=1, column=0, sticky="w", pady=(4, 0))
            return frame

        if block.kind == "image":
            return self.build_image_widget(str(block.data["src"]), str(block.data.get("alt", "")))

        if block.kind == "video":
            return self.build_video_widget(str(block.data["src"]), str(block.data.get("caption", "")))

        if block.kind == "code":
            return self.build_code_widget(
                str(block.data.get("language", "text")),
                [str(item) for item in block.data.get("libraries", [])],
                str(block.data.get("code", "")),
            )

        return None

    def resolve_media_path(self, reference: str) -> Path | None:
        if is_remote_url(reference):
            return None
        base_dir = self.current_file.parent if self.current_file else TUTORIALS_DIR
        path = (base_dir / reference).resolve()
        if path.exists():
            return path
        return None

    def build_image_widget(self, reference: str, alt: str):
        frame = ttk.Frame(self.preview_content)
        path = self.resolve_media_path(reference)

        if path and path.suffix.lower() in {".png", ".gif", ".ppm", ".pgm"}:
            try:
                image = tk.PhotoImage(file=str(path))
                self.preview_images.append(image)
                label = ttk.Label(frame, image=image)
                label.grid(row=0, column=0, sticky="w")
            except tk.TclError:
                ttk.Label(frame, text=f"Image preview unavailable for {path.name}", foreground="#b45309").grid(row=0, column=0, sticky="w")
        else:
            ttk.Label(frame, text=f"Image: {reference}", foreground="#2563eb").grid(row=0, column=0, sticky="w")
            if path:
                ttk.Button(frame, text="Open image", command=lambda p=path: webbrowser.open(p.as_uri())).grid(row=0, column=1, padx=(8, 0))
            elif is_remote_url(reference):
                ttk.Button(frame, text="Open image", command=lambda url=reference: webbrowser.open(url)).grid(row=0, column=1, padx=(8, 0))

        if alt:
            ttk.Label(frame, text=alt, foreground="#64748b", wraplength=420, justify="left").grid(row=1, column=0, columnspan=2, sticky="w", pady=(6, 0))
        return frame

    def build_video_widget(self, reference: str, caption: str):
        frame = ttk.Frame(self.preview_content)
        ttk.Label(frame, text=f"Video: {reference}", wraplength=420, justify="left").grid(row=0, column=0, sticky="w")
        if is_remote_url(reference):
            ttk.Button(frame, text="Open video", command=lambda url=reference: webbrowser.open(url)).grid(row=0, column=1, padx=(8, 0))
        else:
            path = self.resolve_media_path(reference)
            if path:
                ttk.Button(frame, text="Open video", command=lambda p=path: webbrowser.open(p.as_uri())).grid(row=0, column=1, padx=(8, 0))
        if caption:
            ttk.Label(frame, text=caption, foreground="#64748b", wraplength=420, justify="left").grid(row=1, column=0, columnspan=2, sticky="w", pady=(6, 0))
        return frame

    def build_code_widget(self, language: str, libraries: list[str], code: str):
        frame = ttk.Frame(self.preview_content)
        frame.columnconfigure(0, weight=1)

        header = ttk.Frame(frame)
        header.grid(row=0, column=0, sticky="ew", pady=(0, 6))
        header.columnconfigure(1, weight=1)
        ttk.Label(header, text=language.upper(), font=("Arial", 10, "bold"), foreground="#1d4ed8").grid(row=0, column=0, sticky="w")
        if libraries:
            ttk.Label(header, text=f"Libraries: {', '.join(libraries)}", foreground="#64748b", wraplength=320, justify="left").grid(row=0, column=1, sticky="e")

        code_view = tk.Text(
            frame,
            wrap="none",
            height=max(4, min(18, code.count("\n") + 1)),
            font=("Menlo", 11),
            background="#0f172a",
            foreground="#e2e8f0",
            relief="flat",
            padx=10,
            pady=10,
        )
        code_view.grid(row=1, column=0, sticky="ew")
        code_view.insert("1.0", code)
        code_view.configure(state="disabled")
        return frame

    def show_preview_error(self, message: str) -> None:
        for child in self.preview_content.winfo_children():
            child.destroy()
        ttk.Label(self.preview_content, text="Preview could not be generated.", foreground="#b91c1c", font=("Arial", 14, "bold")).grid(row=0, column=0, sticky="w")
        ttk.Label(self.preview_content, text=message, wraplength=420, justify="left").grid(row=1, column=0, sticky="w", pady=(8, 0))
        self.html_preview.configure(state="normal")
        self.html_preview.delete("1.0", tk.END)
        self.html_preview.insert("1.0", message)
        self.html_preview.configure(state="disabled")

    def open_preview_in_browser(self) -> None:
        if self.preview_html_path and self.preview_html_path.exists():
            webbrowser.open(self.preview_html_path.as_uri())
        else:
            messagebox.showinfo("Preview unavailable", "Generate a preview first.")

    def on_preview_content_configure(self, _event=None) -> None:
        self.preview_canvas.configure(scrollregion=self.preview_canvas.bbox("all"))

    def on_preview_canvas_configure(self, event) -> None:
        self.preview_canvas.itemconfigure(self.preview_window, width=event.width)


def main() -> None:
    app = TutorialEditor()
    app.mainloop()


if __name__ == "__main__":
    main()
