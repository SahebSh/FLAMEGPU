# Agent Navigation Grid Plan Editor

`PlanEditManager` is associated with `Window1` (main window) and is the main class for handling the process of Navigation Map building. It handles in the interaction of the main UI. When the user presses `Build map`, the `buildMap` function is called and the inputs are rasterised to an image of size defined by the `Width` and `Height` field. The data is passed to the `NavMap`.

`NavMap` (associated to `MapBuilderWindow`) then handles the process of converting the user's drawings to navigation fields and exporting them to XML ready for FLAME GPU to use. Optionally, `NavMap` can create a `MapResultWindow` for visualising the navigation vectors when the user presses `Build it!`.
