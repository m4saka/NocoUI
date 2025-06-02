# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# NocoUI Development Guide

## Build Commands
- Build project: `cmake -B editor/NocoEditor/build -S editor/NocoEditor && cmake --build editor/NocoEditor/build`
- Run: `cd editor/NocoEditor && ./NocoEditor`
- Debug: Set build type with `cmake -DCMAKE_BUILD_TYPE=Debug` for debugging

## Code Style
- Use C++20 standard features
- Indent with tabs (size 4)
- Use camelCase for methods and variables, PascalCase for classes
- Prefix member variables with `m_` (e.g., `m_canvas`)
- Use Siv3D types and conventions where applicable
- Use YesNo template for boolean types (e.g., `ActiveYN`, `RefreshesLayoutYN`)

## Project Structure
- `include/NocoUI`: Public API headers
- `src`: Implementation files
- `editor/NocoEditor`: UI editor application

## Core Architecture

### Node-Component System
NocoUI uses a hierarchical node system where each Node can have multiple Components attached. This is similar to Unity's GameObject-Component pattern:
- **Node**: Core entity with transform, constraints, children, and interaction state
- **Components**: Functionality modules (RectRenderer, Label, Sprite, TextBox, etc.)
- **Shared ownership**: All nodes and components use `std::shared_ptr` for memory management
- Create components using `node->emplaceComponent<T>(args)`
- Extend ComponentBase or SerializableComponentBase for new component types

### Property System
- **PropertyValue<T>**: State-sensitive values that change based on interaction states (default, hovered, pressed, disabled, selected)
- Supports automatic smoothing/animation with configurable smooth time
- Used for colors, sizes, and other visual properties that need to respond to user interaction

### Layout System
Three layout types available via LayoutVariant:
- **FlowLayout**: Wrapping flow layout for dynamic content
- **HorizontalLayout**: Single-row horizontal arrangement with spacing
- **VerticalLayout**: Single-column vertical arrangement with spacing

### Constraint System
Two constraint types available via ConstraintVariant:
- **BoxConstraint**: Fixed/flexible sizing with margins and flexible weights
- **AnchorConstraint**: Anchor-based positioning relative to parent bounds

### Canvas System
- **Canvas**: Root rendering context that manages the node hierarchy
- Handles global state (hovered nodes, scrollable nodes, text editing)
- Provides event system and coordinate transformations
- Entry point for rendering and update cycles