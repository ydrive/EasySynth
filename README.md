# EasySynth

EasySynth is an Unreal Engine plugin for the easy creation of image datasets from a moving camera, requiring no C++ or Blueprint knowledge.

The plugin works by automatically starting the rendering of a user-defined level sequence, with different camera post-process settings, to produce:

- Camera poses, including position and rotation, as well as calibration parameters
- Standard color images, as seen while creating the sequence in the editor

<img src="ReadmeContent/ColorImage.gif" alt="Color image" width="250"/>

- Depth images, representing the depth of a pixel using a grayscale value

<img src="ReadmeContent/DepthImage.gif" alt="Depth image" width="250"/>

- Normal images, representing pixel normals using X, Y, and Z color values

<img src="ReadmeContent/NormalImage.gif" alt="Normal image" width="250"/>

- Optical flow images, for more detail check out the optical flow section below

<img src="ReadmeContent/OpticalFlowImage.gif" alt="Optical flow image" width="250"/>

- Semantic images, with every object rendered using the user-defined semantic color

<img src="ReadmeContent/SemanticImage.gif" alt="Sematic image" width="250"/>

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

The first step is to define the needed semantic classes, which you can modify at any point. To open the semantic classes editor, click the `Manage Semantic Classes` button. All modifications execute immediately. To close the editor, click the `Done` button.

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

Image rendering relies on a user-defined `Level Sequence`, which represents a movie cut scene inside Unreal Engine. You only need the LevelSequence asset for rendering. Skip anything that has to do with the LevelSequenceActor. Here are some materials on how to get started:
- https://docs.unrealengine.com/4.27/en-US/AnimatingObjects/Sequencer/Overview/
- https://youtu.be/-NmHXAFX-3M

Setting up rendering options inside the EasySynth widget:

![Plugin widget](/ReadmeContent/Widget.png)

- Pick the created level sequence
- Choose the desired rendering targets using checkboxes
- Choose the output image format for each target
  - jpg - 8-bit image output with lossy jpeg compression
  - png - 8-bit image output with lossless png compression, but with zero alpha, making the images appear transparent when previewed
  - exr - 16-bit image output with lossless exr compression, to open them with OpenCV in Python use `cv2.imread(img_path, cv2.IMREAD_ANYCOLOR | cv2.IMREAD_ANYDEPTH)`
- Choose the output images width and height
  - Choosing a different aspect ratio than the selected camera actor aspect ratio can result in an unexpected field of view in the output images
- Choose the depth infinity threshold for depth rendering
- Choose the appropriate scaling coefficient for increasing optical flow image color saturation
- Choose the output directory

Start the rendering by clicking the `Render Images` button.

<b>IMPORTANT:</b> Do not close a window that opens during rendering. Closing the window will result in the successful rendering being falsely reported, as it is not possible to know if the window has been closed from the plugin side.

### Level sequence tips

- If you want images to be more spaced out (instead of being very close to provide a smooth video), you can set the custom level sequence FPS to a small value (1 FPS or lower) inside the Sequencer editor

### Camera pose output

If requested, the plugin exports camera poses to the same output directory as rendered images.

Output is the `CameraPoses.csv` file, in which the first line contains column names and the rest contain camera poses for each frame. Columns are the following:

| Column | Type  | Name | Description                |
| ------ | ----- | ---- | -------------------------- |
| 1      | int   | id   | 0-indexed frame id         |
| 2      | float | tx   | X position in meters       |
| 3      | float | ty   | Y position in meters       |
| 4      | float | tz   | Z position in meters       |
| 5      | float | qw   | Rotation quaternion W      |
| 6      | float | qx   | Rotation quaternion X      |
| 7      | float | qy   | Rotation quaternion Y      |
| 8      | float | qz   | Rotation quaternion Z      |
| 9      | float | fx   | Focal length X in pixels   |
| 10     | float | fy   | Focal length Y - same as X |
| 11     | int   | cx   | Halved image width         |
| 12     | int   | cy   | Halved image height        |

The coordinate system for saving camera positions and rotation quaternions is the usual right-handed Z-up coordinate system (consistent with Blender and 3ds Max). Note that this differs from Unreal Engine, which internally uses the left-handed Z-up coordinate system.

### Optical flow images

Optical flow images contain color-coded optical flow vectors for each pixel. An optical flow vector describes how the content of a pixel has moved between frames. Specifically, the vector spans from coordinates where the pixel content was in the previous frame to where the content is in the current frame. The coordinate system the vectors are represented in is the image pixel coordinates, with the image scaled to a 1.0 x 1.0 square.

Optical flow vectors are color-coded by picking a color from the HSV color wheel with the color angle matching the vector angle and the color saturation matching the vector intensity. If the scene in your sequence moves slowly, these vectors can be very short, and the colors can be hard to see when previewed. If this is the case, use the `optical flow scale` parameter to proportionally increase the images saturation.

Our implementation was inspired by the [ProfFan's](https://github.com/ProfFan) [UnrealOpticalFlowDemo](https://github.com/ProfFan/UnrealOpticalFlowDemo), but we had to omit the engine patching to make this plugin as easy to use as possible (i.e. not requiring the engine to be manually built). The shader code that renders optical flow is baked inside the optical flow post-process material. It can be accessed by opening the post-process material from the plugin's content inside the Unreal Engine editor.

<b>IMPORTANT:</b> Due to Unreal Engine [limitations](https://github.com/EpicGames/UnrealEngine/pull/6933) optical flow rendering assumes all objects other than the camera are stationary. If there are moving objects in the scene while rendering the sequence, the optical flow for these pixels will be incorrect.
