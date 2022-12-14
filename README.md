# Dino Dino!

![Screenshot](screenshot.png)

**Dino Dino!** is a tiny dinosaur collecting game built in [The Machinery](https://ourmachinery.com/).

To run it you need [The Machinery 2021.2.a](https://ourmachinery.com/beta.html). Open `project/project.the_machinery_dir` in The Machinery and then open a *Simulate tab* to run the game.

You can also download a prebuilt binary from [itch.io](https://ourmachinery.itch.io/dino-dino).

This game, the code and the art are licensed under the [CC BY-NC
4.0](https://creativecommons.org/licenses/by-nc/4.0/) license.

You can find a game design spreadsheet with the rules of the game here:

* https://docs.google.com/spreadsheets/d/11sT_7U7IMrL_BpgIoLGul436z4L0lZe-oSCdbEn09DU/edit?usp=sharing

You can change the rules by changing the values in the spreadsheet and copying the C code columns
into the source code.

To compile the source code, open the `src` directory in Visual Studio Code and run `Tasks: Run Build
Task`. (You need to set the `TM_DINO_SDK_DIR` environment variable to point to your `The Machinery
2021.2.a` installation folder.)

You can set The Machinery up to automatically reload your code every time you build it. To do so,
select `tm_dinosaur_simulate.plugin` in the asset browser and click on the **Windows DLL Path**
object to point it to the location of your built `.dll`. Also make sure that **Import When Changed**
is selected:

![Plugin properties](plugin-properties.png)
