# Annotation Demo Plan

This document describes a short screen-recording plan for presenting the annotation feature.

## Goal

Show the annotation workflow quickly and clearly, using one dataset and a small number of actions.

## Recommended Dataset

Use:

- [datasamples/annotations_demo.csv](/f:/src/PlotJuggler/datasamples/annotations_demo.csv)

With autoload enabled, also show:

- [datasamples/annotations_demo.annotations.json](/f:/src/PlotJuggler/datasamples/annotations_demo.annotations.json)

## Recording Length

Recommended:

- 60 to 120 seconds

## Suggested Story

### Part 1: Load And Autoload

Show:

1. Load `annotations_demo.csv`
2. Let the companion annotation file autoload
3. Expand the tree
4. Show peak markers and the reference region on the plot

### Part 2: Manual Editing

Show:

1. Create a new group
2. Add one point annotation
3. Add one region annotation
4. Edit label or notes
5. Toggle visibility from the tree

### Part 3: Tree Interaction

Show:

1. Drag an annotation into a group
2. Copy and paste a node
3. Multi-select and delete or copy
4. Show tri-state visibility on a parent group

### Part 4: Generation

Show:

1. Click `Generate`
2. Use `Regions from threshold` on `y_true`
3. Create a generated annotation file
4. Use `Regions from integer mask match` on `status_flags`
5. Show a mask example such as:
   - `mask = 0x01`
   - `match = 0x01`

### Part 5: Export

Show:

1. Copy selected annotations
2. Paste into a spreadsheet or text editor

## Plot Suggestions

For the clearest recording, display:

- `disturbed`
- `sine_a`
- `sine_b`
- `y_true`
- optionally `status_flags`

This makes it easy to see:

- analog behavior
- disturbance windows
- generated regions
- reference markers

## Recording Tips

- keep the tree expanded enough to show structure
- avoid too many files or tabs
- prefer one clean happy-path flow
- zoom in before editing handles
- keep labels readable

## What To Emphasize

Good talking points:

- manual and generated annotations can coexist
- annotations can be organized hierarchically
- visibility works on subtrees
- generated annotations can come from simple signals without scripting
- spreadsheet copy/paste is supported
