#!/usr/bin/env python3
from __future__ import annotations

import sys
import tkinter as tk
import webbrowser
from pathlib import Path
from tkinter import filedialog, messagebox, simpledialog, ttk

ROOT_DIR = Path(__file__).resolve().parents[1]
TUTORIALS_DIR = ROOT_DIR / "tutorials"
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from _scripts.build_tutorials import is_remote_url, parse_tutorial, render_page

PREVIEW_WORK_DIR = ROOT_DIR / "build" / "tutorial-editor-preview"
PREVIEW_SITE_DIR = PREVIEW_WORK_DIR / "site"
DEFAULT_TEMPLATE = """@title1: New CoolBox Tutorial
@tags: tutorial

Write your introduction here.
"""


class TutorialEditor(tk.Tk):
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

        self._build_layout()
        self.load_template()
        self.refresh_preview()

    def _build_layout(self) -> None:
        self.columnconfigure(0, weight=1)
        self.rowconfigure(1, weight=1)

        toolbar = ttk.Frame(self, padding=(12, 10))
        toolbar.grid(row=0, column=0, sticky="ew")
        for idx in range(12):
            toolbar.columnconfigure(idx, weight=0)
        toolbar.columnconfigure(12, weight=1)

        ttk.Button(toolbar, text="New", command=self.new_file).grid(row=0, column=0, padx=4)
        ttk.Button(toolbar, text="Open", command=self.open_file).grid(row=0, column=1, padx=4)
        ttk.Button(toolbar, text="Save", command=self.save_file).grid(row=0, column=2, padx=4)
        ttk.Button(toolbar, text="Save As", command=self.save_file_as).grid(row=0, column=3, padx=4)
        ttk.Separator(toolbar, orient="vertical").grid(row=0, column=4, sticky="ns", padx=8)
        ttk.Button(toolbar, text="Title", command=lambda: self.insert_title(1)).grid(row=0, column=5, padx=4)
        ttk.Button(toolbar, text="Subtitle", command=lambda: self.insert_title(2)).grid(row=0, column=6, padx=4)
        ttk.Button(toolbar, text="Image", command=self.insert_image).grid(row=0, column=7, padx=4)
        ttk.Button(toolbar, text="Video", command=self.insert_video).grid(row=0, column=8, padx=4)
        ttk.Button(toolbar, text="Link", command=self.insert_link).grid(row=0, column=9, padx=4)
        ttk.Button(toolbar, text="Tags", command=self.insert_tags).grid(row=0, column=10, padx=4)
        ttk.Button(toolbar, text="Paragraph", command=self.insert_paragraph).grid(row=0, column=11, padx=4)

        self.status_var = tk.StringVar(value="Ready")
        ttk.Label(toolbar, textvariable=self.status_var, anchor="e").grid(row=0, column=12, sticky="ew", padx=(12, 0))

        content = ttk.Panedwindow(self, orient=tk.HORIZONTAL)
        content.grid(row=1, column=0, sticky="nsew", padx=12, pady=(0, 12))

        editor_frame = ttk.Frame(content, padding=(0, 0, 8, 0))
        editor_frame.columnconfigure(0, weight=1)
        editor_frame.rowconfigure(1, weight=1)
        ttk.Label(editor_frame, text=".tut Source", font=("Arial", 13, "bold")).grid(row=0, column=0, sticky="w", pady=(0, 8))

        self.text = tk.Text(editor_frame, wrap="word", undo=True, font=("Menlo", 12))
        self.text.grid(row=1, column=0, sticky="nsew")
        editor_scroll = ttk.Scrollbar(editor_frame, orient="vertical", command=self.text.yview)
        editor_scroll.grid(row=1, column=1, sticky="ns")
        self.text.configure(yscrollcommand=editor_scroll.set)
        self.text.bind("<<Modified>>", self.on_text_modified)

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
            "@tags: docs, intro, tutorial\n\n"
            "@image: assets/example.png | Optional alt text\n"
            "@video: https://example.com/video.mp4 | Optional caption\n"
            "@link: https://example.com | Link label\n\n"
            "Plain text lines become paragraphs. Blank lines separate paragraphs.\n\n"
            "Tips\n"
            "• Save tutorials into the top-level tutorials/ folder.\n"
            "• Keep local media next to the tutorial file or in tutorials/assets/.\n"
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
        self.text.delete("1.0", tk.END)
        self.text.insert("1.0", path.read_text(encoding="utf-8"))
        self.current_file = path
        self.text.edit_modified(False)
        self.is_dirty = False
        self.status_var.set(f"Opened {path.name}")
        self.refresh_preview()

    def save_file(self) -> bool:
        if self.current_file is None:
            return self.save_file_as()
        self.current_file.write_text(self.text.get("1.0", tk.END).rstrip() + "\n", encoding="utf-8")
        self.text.edit_modified(False)
        self.is_dirty = False
        self.status_var.set(f"Saved {self.current_file.name}")
        self.refresh_preview()
        return True

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
        return self.save_file()

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
            self.insert_text(f"@tags: {tags}\n")
            self.status_var.set("Inserted tags")

    def insert_image(self) -> None:
        path = simpledialog.askstring("Insert image", "Image path or URL:", parent=self)
        if not path:
            return
        alt = simpledialog.askstring("Insert image", "Alt text (optional):", parent=self) or ""
        payload = f"@image: {path}" + (f" | {alt}" if alt else "")
        self.insert_text(payload + "\n")
        self.status_var.set("Inserted image")

    def insert_video(self) -> None:
        path = simpledialog.askstring("Insert video", "Video path or URL:", parent=self)
        if not path:
            return
        caption = simpledialog.askstring("Insert video", "Caption (optional):", parent=self) or ""
        payload = f"@video: {path}" + (f" | {caption}" if caption else "")
        self.insert_text(payload + "\n")
        self.status_var.set("Inserted video")

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

    def on_text_modified(self, _event: tk.Event[tk.Text]) -> None:
        if self.text.edit_modified():
            self.is_dirty = True
            self.text.edit_modified(False)
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
