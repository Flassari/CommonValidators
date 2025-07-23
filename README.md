# CommonValidators
Collection of validators that are useful for all projects.

# Installation
Copy this plugin to either your PRROJECT_DIR/Plugins or ENGINE_DIR/Plugins folder.

# How it works
This plugin adds three new validators to the Data Validation system in Unreal Engine. These validators will run automatically when you save an asset.

## EditorValidator_BlockingLoad
This validator scans Blueprints for nodes that perform synchronous (blocking) asset loading, which can cause hitches and performance issues. It specifically looks for `LoadAsset_Blocking` and `LoadClassAsset_Blocking` function calls.

## EditorValidator_EmptyTick
This validator identifies `EventTick` nodes in Blueprints that are not connected to any other nodes (i.e., they do nothing). These empty ticks still have a performance cost, so the validator flags them for removal. This check is only active for Unreal Engine versions older than 5.6.

## EditorValidator_PureNode
This validator checks for pure nodes (nodes without execution pins) that have more than one output pin connected. This is important because a pure node is re-executed for each connected output, which can be an unexpected and inefficient behavior. It includes a whitelist to exclude certain node types, like `UK2Node_BreakStruct`, from this validation.
