# Annotation Manual

This document is a practical guide to the annotation feature in PlotJuggler.

## Purpose

Annotations can be used to:

- mark interesting points in time
- mark time intervals
- organize manual and generated annotations in groups
- save annotations in companion files next to the data
- copy annotations to other tools such as spreadsheets

## Main Concepts

### Annotation File

An annotation file is the top-level container.

It can contain:

- groups
- point annotations
- region annotations

Annotation files are usually stored as sidecar JSON files with names like:

- `session.annotations.json`

If autoload is enabled, companion annotation files next to the loaded data file are loaded automatically.

### Group

A group is a tree node used to organize annotations.

Groups can:

- contain subgroups
- contain annotations
- have a name
- have a color
- control visibility for their descendants

### Annotation

An annotation is either:

- a `Point`
- a `Region`

Each annotation can have:

- label
- tags
- notes
- enabled state
- optional color override

## Basic Workflow

### Create Or Load

At the top of the Annotations panel:

- `New` creates a new annotation file
- `Load` loads an existing annotation file
- `Generate` creates annotations directly from signal data

### Add Annotations Manually

Common ways to add annotations:

- use `+P` to add a point annotation
- use `+R` to add a region annotation
- use `+G` to add a group
- use the context menu on a file or group

### Edit Annotations

Select an annotation in the tree and edit:

- `Label`
- `Tags`
- `Notes`
- `Enabled`

### File And Group Properties

Use:

- `Prop` on a file or group row
- `F2`
- context menu

Use file properties to edit:

- file name
- file color

Use group properties to edit:

- group name
- group color

## Tree Behavior

The annotation tree supports:

- nested groups
- drag and drop
- copy / cut / paste
- multi-selection for bulk operations
- tri-state visibility

Tri-state visibility works like this:

- checking a parent enables the subtree
- unchecking a parent disables the subtree
- mixed child state makes the parent partially checked

## Context Menu And Shortcuts

Useful shortcuts:

- `Insert`: add point annotation
- `Shift+Insert`: add region annotation
- `Ctrl+G`: add group
- `Ctrl+Shift+G`: open generator
- `F2`: properties / rename
- `Delete`: delete selected nodes
- `Ctrl+C`: copy
- `Ctrl+X`: cut
- `Ctrl+V`: paste
- `Ctrl+D`: duplicate annotation
- `Enter`: jump to annotation
- `Ctrl+S`: save
- `Ctrl+Shift+S`: save as

## Copy And Paste

Copy and paste supports:

- internal PlotJuggler tree copy / paste
- JSON clipboard data
- flat TSV export for spreadsheets

This means you can:

- copy annotations inside PlotJuggler
- paste flat rows into Excel
- copy edited TSV data back into PlotJuggler

## Built-In Generators

The current generators are available from:

- `Generate`
- file context menu
- group context menu
- `Ctrl+Shift+G`

### Regions From Threshold

Creates active regions when a numeric signal matches a threshold condition.

Examples:

- `value >= threshold`
- `value > threshold`
- `value <= threshold`
- `value < threshold`

Useful for signals like:

- `y_true`
- binary filters
- thresholded analog channels

### Regions From Integer Mask Match

Creates active regions when an integer-like signal matches:

- `(signal & mask) == match_value`

Hex constants are supported, for example:

- `mask = 0xF0`
- `match = 0x20`

Useful for:

- status flags
- bit fields
- packed state values

### Generator Options

The current generator dialog supports:

- source signal
- generator type
- threshold or mask/match parameters
- minimum duration
- maximum number of generated regions
- target:
  - new annotation file
  - current tree target
- target naming
- live preview count

## Visibility And Color Inheritance

Color should normally be set high in the tree.

The intended model is:

- file color is the default
- group color overrides the file for that subtree
- annotation color overrides the inherited color only when explicitly set

This makes it possible to recolor a generated set from the top level without editing every annotation.

## Out-Of-Range Annotations

Annotations outside the current session time range are:

- kept in the tree
- marked as out of range
- not drawn on the plot
- prevented from affecting zoom/range behavior

## Demo Data

Recommended demo file:

- [datasamples/annotations_demo.csv](/f:/src/PlotJuggler/datasamples/annotations_demo.csv)

Matching companion file:

- [datasamples/annotations_demo.annotations.json](/f:/src/PlotJuggler/datasamples/annotations_demo.annotations.json)

This demo contains:

- analog sine-like signals
- a disturbed analog signal
- `y_true` for threshold-based region generation
- `status_flags` for mask-match region generation

Other available sample files:

- [datasamples/simple.csv](/f:/src/PlotJuggler/datasamples/simple.csv)
- [datasamples/motor_data.csv](/f:/src/PlotJuggler/datasamples/motor_data.csv)
- [datasamples/turtle.csv](/f:/src/PlotJuggler/datasamples/turtle.csv)
- [datasamples/fake_joints.csv](/f:/src/PlotJuggler/datasamples/fake_joints.csv)

## Current Scope

The current implementation supports:

- hierarchical annotation trees
- manual editing
- generated annotations
- drag/drop and clipboard workflows

The current implementation does not yet try to be:

- a full review workflow tool
- a Lua-based generator system
- a final model/view rewrite using `QTreeView`
