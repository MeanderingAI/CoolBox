#import <Cocoa/Cocoa.h>
#include "cocoa_native_menu.hpp"
#include <algorithm>
#include <utility>
#include <vector>

@interface CoolBoxMenuTarget : NSObject {
@public
    graphics::full_application_window::detail::CocoaMenuCommand command;
    NSMenu* menu;
    NSWindow* owner;
}
- (void)activate:(NSMenuItem*)sender;
- (void)becameKey:(NSNotification*)notification;
@end

namespace {
std::vector<CoolBoxMenuTarget*> activeMenus;
NSMenu* originalMenu = nil;
}

@implementation CoolBoxMenuTarget
- (void)activate:(NSMenuItem*)sender {
    NSArray* indices = [sender representedObject];
    // Copy before invoking: the handler may rebuild and release this target.
    auto callback = command;
    if (callback) callback([[indices objectAtIndex:0] unsignedLongValue],
                           [[indices objectAtIndex:1] unsignedLongValue]);
}
- (void)becameKey:(NSNotification*)notification {
    [NSApp setMainMenu:menu];
}
- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    activeMenus.erase(std::remove(activeMenus.begin(), activeMenus.end(), self), activeMenus.end());
    if ([NSApp mainMenu] == menu)
        [NSApp setMainMenu:activeMenus.empty() ? originalMenu : activeMenus.back()->menu];
    if (activeMenus.empty()) {
        [originalMenu release];
        originalMenu = nil;
    }
    [menu release];
    [super dealloc];
}
@end

namespace graphics::full_application_window::detail {
void* create_cocoa_menu(void* window, const components::MenuBarModel& model,
                        CocoaMenuCommand command) {
    @autoreleasepool {
    CoolBoxMenuTarget* target = [[CoolBoxMenuTarget alloc] init];
    target->command = std::move(command);
    target->owner = static_cast<NSWindow*>(window);
    if (activeMenus.empty()) originalMenu = [[NSApp mainMenu] retain];
    activeMenus.push_back(target);
    target->menu = [[NSMenu alloc] initWithTitle:@""];
    [target->menu setAutoenablesItems:NO];
    // AppKit reserves the first menu for the application menu.
    NSMenuItem* appItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* quit = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenu addItem:quit];
    [quit release];
    [appItem setSubmenu:appMenu];
    [appMenu release];
    [target->menu addItem:appItem];
    [appItem release];
    for (std::size_t m = 0; m < model.menus.size(); ++m) {
        const auto& source = model.menus[m];
        NSString* title = [NSString stringWithUTF8String:source.title.c_str()];
        NSMenuItem* top = [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];
        NSMenu* submenu = [[NSMenu alloc] initWithTitle:title];
        [submenu setAutoenablesItems:NO];
        for (std::size_t i = 0; i < source.items.size(); ++i) {
            const auto& item = source.items[i];
            if (item.separator) { [submenu addItem:[NSMenuItem separatorItem]]; continue; }
            NSString* shortcut = [NSString stringWithUTF8String:item.shortcut.c_str()];
            NSArray* parts = [shortcut componentsSeparatedByString:@"+"];
            NSEventModifierFlags flags = 0;
            NSString* key = [[parts lastObject] lowercaseString];
            for (NSUInteger p = 0; p + 1 < [parts count]; ++p) {
                NSString* modifier = [[parts objectAtIndex:p] lowercaseString];
                if ([modifier isEqualToString:@"ctrl"] || [modifier isEqualToString:@"cmd"] ||
                    [modifier isEqualToString:@"command"]) flags |= NSEventModifierFlagCommand;
                if ([modifier isEqualToString:@"shift"]) flags |= NSEventModifierFlagShift;
                if ([modifier isEqualToString:@"alt"] || [modifier isEqualToString:@"option"]) flags |= NSEventModifierFlagOption;
            }
            if ([key isEqualToString:@"space"]) key = @" ";
            if ([key isEqualToString:@"backspace"]) key = @"\x7f";
            if ([key isEqualToString:@"delete"]) key = @"\uf728";
            if ([key isEqualToString:@"enter"] || [key isEqualToString:@"return"]) key = @"\r";
            if ([key isEqualToString:@"escape"] || [key isEqualToString:@"esc"]) key = @"\x1b";
            if ([key isEqualToString:@"left"]) key = @"\uf702";
            if ([key isEqualToString:@"right"]) key = @"\uf703";
            if ([key isEqualToString:@"home"]) key = @"\uf729";
            if ([key isEqualToString:@"end"]) key = @"\uf72b";
            NSMenuItem* native = [[NSMenuItem alloc]
                initWithTitle:[NSString stringWithUTF8String:item.label.c_str()]
                action:@selector(activate:) keyEquivalent:key];
            [native setKeyEquivalentModifierMask:flags];
            [native setEnabled:item.enabled];
            [native setState:item.checked ? NSControlStateValueOn : NSControlStateValueOff];
            [native setTarget:target];
            [native setRepresentedObject:@[@(m), @(i)]];
            [submenu addItem:native];
            [native release];
        }
        [top setSubmenu:submenu];
        [submenu release];
        [target->menu addItem:top];
        [top release];
    }
    [[NSNotificationCenter defaultCenter] addObserver:target selector:@selector(becameKey:)
        name:NSWindowDidBecomeKeyNotification object:target->owner];
    [NSApp setMainMenu:target->menu];
    return target;
    }
}
void destroy_cocoa_menu(void* menu) {
    [static_cast<CoolBoxMenuTarget*>(menu) release];
}
}
