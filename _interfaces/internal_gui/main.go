package main

import (
	"bufio"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"regexp"
	"strings"

	"fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/container"
	"fyne.io/fyne/v2/widget"
	"github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"
)

type MakeOption struct {
	Target  string
	Comment string
}

type Tab struct {
	Name    string
	Options []MakeOption
}

func parseMakefile(path string) ([]MakeOption, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	var options []MakeOption
	var lastComment string
	targetRe := regexp.MustCompile(`^([a-zA-Z0-9_-]+):`) // target: line
	for scanner.Scan() {
		line := scanner.Text()
		if strings.HasPrefix(strings.TrimSpace(line), "#") {
			lastComment = strings.TrimSpace(strings.TrimPrefix(line, "#"))
		} else if m := targetRe.FindStringSubmatch(line); m != nil {
			options = append(options, MakeOption{Target: m[1], Comment: lastComment})
			lastComment = ""
		}
	}
	return options, scanner.Err()
}

// Categorize Makefile targets into tabs
func categorizeOptions(options []MakeOption) []Tab {
	var apps, services, libs, demos, tests []MakeOption
	for _, opt := range options {
		name := opt.Target
		// Only classify as demo or test if not also an app, service, or lib
		if strings.Contains(name, "app") || strings.HasPrefix(name, "run-") || strings.HasPrefix(name, "build-app") {
			apps = append(apps, opt)
		} else if strings.Contains(name, "service") {
			services = append(services, opt)
		} else if strings.Contains(name, "lib") || strings.Contains(name, "_libraries") {
			libs = append(libs, opt)
		} else if strings.Contains(name, "demo") {
			demos = append(demos, opt)
		} else if strings.Contains(name, "test") {
			tests = append(tests, opt)
		}
	}
	return []Tab{
		{Name: "Apps", Options: apps},
		{Name: "Services", Options: services},
		{Name: "Library", Options: libs},
		{Name: "Demo", Options: demos},
		{Name: "Unit Tests", Options: tests},
	}
}

func main() {
	guiFlag := flag.Bool("gui", false, "Launch graphical UI instead of terminal UI")
	flag.Parse()

	options, err := parseMakefile("../Makefile")
	if err != nil {
		fmt.Println("Error reading Makefile:", err)
		os.Exit(1)
	}

	tabs := categorizeOptions(options)

	if *guiFlag {
		runGUI(tabs)
	} else {
		runTUI(tabs)
	}
}

func runTUI(tabs []Tab) {
	app := tview.NewApplication()
	tabBar := tview.NewTextView().SetDynamicColors(true)
	list := tview.NewList()
	descModal := tview.NewModal().SetText("").AddButtons([]string{"Close"})

	currentTab := 0
	updateTabBar := func() {
		var bar string
		for i, t := range tabs {
			if i == currentTab {
				bar += "[yellow]" + t.Name + "[white] "
			} else {
				bar += t.Name + " "
			}
		}
		tabBar.SetText(bar)
	}

	updateList := func() {
		list.Clear()
		opts := tabs[currentTab].Options
		for i, opt := range opts {
			label := opt.Target
			if opt.Comment != "" {
				label += " - " + opt.Comment
			}
			idx := i // capture for closure
			list.AddItem(label, "", 0, func() {
				cmd := exec.Command("make", opts[idx].Target)
				cmd.Stdout = os.Stdout
				cmd.Stderr = os.Stderr
				cmd.Run()
			})
		}
	}

	updateTabBar()
	updateList()

	flex := tview.NewFlex().SetDirection(tview.FlexRow).
		AddItem(tabBar, 1, 0, false).
		AddItem(list, 0, 1, true)

	list.SetBorder(true).SetTitle("[::b]Makefile Options").SetTitleAlign(tview.AlignLeft)
	list.SetDoneFunc(func() { app.Stop() })

	list.SetInputCapture(func(event *tcell.EventKey) *tcell.EventKey {
		switch event.Key() {
		case tcell.KeyLeft:
			if currentTab > 0 {
				currentTab--
				updateTabBar()
				updateList()
			}
			return nil
		case tcell.KeyRight:
			if currentTab < len(tabs)-1 {
				currentTab++
				updateTabBar()
				updateList()
			}
			return nil
		}
		switch event.Rune() {
		case 'i', 'm':
			idx := list.GetCurrentItem()
			opts := tabs[currentTab].Options
			if idx >= 0 && idx < len(opts) {
				desc := opts[idx].Comment
				if desc == "" {
					desc = "No description available."
				}
				descModal.SetText("[::b]" + opts[idx].Target + "[-]\n\n" + desc)
				app.SetRoot(descModal, false).SetFocus(descModal)
			}
			return nil
		}
		return event
	})

	descModal.SetDoneFunc(func(buttonIndex int, buttonLabel string) {
		app.SetRoot(flex, true).SetFocus(list)
	})

	if err := app.SetRoot(flex, true).EnableMouse(true).Run(); err != nil {
		fmt.Println(err)
	}
}

func runGUI(tabs []Tab) {
	fmt.Println("Launching Fyne GUI...")
	defer func() {
		if r := recover(); r != nil {
			fmt.Fprintf(os.Stderr, "Fyne GUI crashed: %v\n", r)
		}
	}()
	fyneApp := app.New()
	w := fyneApp.NewWindow("Makefile GUI")

	tabNames := make([]string, len(tabs))
	for idx, t := range tabs {
		tabNames[idx] = t.Name
	}
	tabSelect := widget.NewSelect(tabNames, nil)
	list := widget.NewList(
		func() int { return len(tabs[0].Options) },
		func() fyne.CanvasObject { return widget.NewButton("", nil) },
		func(i int, obj fyne.CanvasObject) {
			btn := obj.(*widget.Button)
			opt := tabs[0].Options[i]
			label := opt.Target
			if opt.Comment != "" {
				label += " - " + opt.Comment
			}
			btn.SetText(label)
			btn.OnTapped = func() {
				go func() {
					cmd := exec.Command("make", opt.Target)
					cmd.Stdout = os.Stdout
					cmd.Stderr = os.Stderr
					cmd.Run()
				}()
			}
		},
	)

	tabSelect.OnChanged = func(name string) {
		for _, t := range tabs {
			if t.Name == name {
				list.Length = func() int { return len(t.Options) }
				list.UpdateItem = func(idx int, obj fyne.CanvasObject) {
					btn := obj.(*widget.Button)
					opt := t.Options[idx]
					label := opt.Target
					if opt.Comment != "" {
						label += " - " + opt.Comment
					}
					btn.SetText(label)
					btn.OnTapped = func() {
						go func() {
							cmd := exec.Command("make", opt.Target)
							cmd.Stdout = os.Stdout
							cmd.Stderr = os.Stderr
							cmd.Run()
						}()
					}
				}
				list.Refresh()
				break
			}
		}
	}

	w.SetContent(container.NewVBox(
		widget.NewLabel("Select Category:"),
		tabSelect,
		widget.NewLabel("Makefile Targets:"),
		list,
	))
	w.Resize(fyne.NewSize(600, 400))
	w.ShowAndRun()
}
