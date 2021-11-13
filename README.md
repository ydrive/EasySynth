# EasySynth

An Unreal Engine plugin for easy creation of image datasets, requiring no C++ or Blueprint knowledge.

The plugin works by automatically starting the rendering of a user-defined level sequence with different camera post process settings, in order to produce:

- Standard color images, as seen while creating the sequence in the editor
- Depth images, representing the depth of a pixel by a grayscale value
- Normal images, representing the pixel normal using X, Y and Z color values

## Installation

### Install by adding the plugin source code to your project

- Create a new Unreal Engine project with the path `<UEProject>`
  - <em>Optional</em>: Use some of the provided Unreal Engine templates, that come with a prebuilt level layout
- <b>IMPORTANT</b> Enable the Movie Render Queue builtin plugin (Edit -> Plugins -> Movie Render Queue -> Enabled)
- Exit the editor
- Create and navigate to the `<UEProject>/Plugins` directory
- Clone this repo inside the `<UEProject>/Plugins` directory
- Reopen your project

## How to use

- Create the project as described in the `Installation` section
- Create a level sequence as described in following resources, only the LevelSequence asset is needed, you can skip anything that has to do with the LevelSequenceActor
  - https://docs.unrealengine.com/4.27/en-US/AnimatingObjects/Sequencer/Overview/
  - https://youtu.be/-NmHXAFX-3M
- Click the EasySynth button in the toolbar to open the EasySynth widget
- In the widget:
  - Pick the created level sequence
  - Choose the desired rendering targets
  - Choose the depth infinite threshold for depth rendering
  - Choose the output directory
  - Click the Render Images button
- Wait for the successful rendering message box
