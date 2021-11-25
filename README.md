# EasySynth

An Unreal Engine plugin for easy creation of image datasets from a moving camera, requiring no C++ or Blueprint knowledge.

The plugin works by automatically starting the rendering of a user-defined level sequence, with different camera post-process settings, in order to produce:

- Standard color images, as seen while creating the sequence in the editor
- Depth images, representing the depth of a pixel by a grayscale value
- Normal images, representing the pixel normal using X, Y and Z color values
- Semantic images, with every object rendered using the user-defined semantic color
- Camera poses, including location and rotation

## Installation

### Install by adding the plugin source code to your project

- Create a new Unreal Engine project with the path `<UEProject>`
  - <em>Optional</em>: Use some of the provided Unreal Engine templates that come with a prebuilt level layout
- <b>IMPORTANT</b> Enable the Movie Render Queue builtin plugin (Edit -> Plugins -> Movie Render Queue -> Enabled)
- Exit the editor
- Create and navigate to the `<UEProject>/Plugins` directory
- Clone this repo inside the `<UEProject>/Plugins` directory
- Reopen your project

## How to use

### Setup

- Create a project as described in the `Installation` section
- Create a level layout yourself, or look online for available levels that suit your needs
- Click the EasySynth button in the toolbar to open the EasySynth widget

### Semantic annotation

You can skip this step if you don't need semantic rendering.

The first step is to define the semantic classes you will use. You can modify them at any point. To open the semantic classes editor, click the `Manage Semantic Classes` button. All modifications execute immediately. To close the editor, click the `Done` button.

This editor allows you to:
- Add a new semantic class
- Remove an existing semantic class
- Modify an existing semantic class name
- Modify an existing semantic class color

Next, you should assign semantic classes to all of the level actors. To do this:
- Select one or more actors of the same class in the editor window or by using the World Outliner
- Assign them a class by clicking on the `Pick a semantic class` button and picking the class

To toggle between original and semantic color, use the `Pick a mesh texture style` button. Make sure that you never save your project while the semantic view mode is selected.

### Sequence rendering

Image rendering relies on a user-defined `Level Sequence`, which represents a movie cut scene inside Unreal Engine. You will only need the LevelSequence asset for rendering. Skip anything that has to do with the LevelSequenceActor. Here are some materials on how to get started:
- https://docs.unrealengine.com/4.27/en-US/AnimatingObjects/Sequencer/Overview/
- https://youtu.be/-NmHXAFX-3M

Setting up rendering options inside the EasySynth widget:
- Pick the created level sequence
- Choose the desired rendering targets using checkboxes
- Choose the depth infinity threshold for depth rendering
- Choose the output directory

Start the rendering by clicking the `Render Images` button.

### Camera pose output

If requested, camera poses are saved inside the same output directory as rendered images.

Output is the `CameraPoses.csv` file, in which the first line contains column names and the rest contain camera poses for each frame. Columns are the following:

| Column | Name | Description           |
| ------ | ---- | --------------------- |
| 1      | id   | 0-indexed frame id    |
| 2      | tx   | X position in meters  |
| 3      | ty   | Y position in meters  |
| 4      | tz   | Z position in meters  |
| 5      | qw   | Rotation quaternion W |
| 6      | qx   | Rotation quaternion X |
| 7      | qy   | Rotation quaternion Y |
| 8      | qz   | Rotation quaternion Z |
