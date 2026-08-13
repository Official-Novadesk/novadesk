import { dialog, app } from "novadesk";

console.log("=========================================");
console.log("   Novadesk MessageBox Integration Test");
console.log("=========================================");
console.log("");

function testDialog(testName, options, expectedType) {
    console.log("[TEST] " + testName);
    console.log("  Options:", JSON.stringify(options));
    
    try {
        const result = dialog.show(options);
        console.log("  [RESULT] User clicked: " + result);
        console.log("  [STATUS] ✓ PASS");
        return result;
    } catch (err) {
        console.error("  [ERROR] " + String(err));
        console.log("  [STATUS] ✗ FAIL");
        return null;
    }
    console.log("");
}

// Test 1: Simple info dialog with OK button
console.log("=== Test Suite: Basic Dialog Types ===");
testDialog(
    "Info Dialog - OK Button",
    {
        title: "Information",
        message: "This is an information dialog.\nClick OK to continue.",
        type: "info",
        buttons: "ok"
    }
);

// Test 2: Warning dialog with OK/Cancel
testDialog(
    "Warning Dialog - OK/Cancel",
    {
        title: "Warning",
        message: "This is a warning dialog.\nChoose OK or Cancel.",
        type: "warning",
        buttons: "ok-cancel"
    }
);

// Test 3: Error dialog
testDialog(
    "Error Dialog - OK Button",
    {
        title: "Error",
        message: "This is an error dialog.\nSomething went wrong!",
        type: "error",
        buttons: "ok"
    }
);

// Test 4: Question dialog with Yes/No
testDialog(
    "Question Dialog - Yes/No",
    {
        title: "Confirm",
        message: "Do you want to proceed with this action?",
        type: "question",
        buttons: "yes-no"
    }
);

console.log("");
console.log("=== Test Suite: Button Combinations ===");

// Test 5: Yes/No/Cancel
testDialog(
    "Question Dialog - Yes/No/Cancel",
    {
        title: "Save Changes",
        message: "Would you like to save your changes?",
        type: "question",
        buttons: "yes-no-cancel"
    }
);

// Test 6: Retry/Cancel
testDialog(
    "Error Dialog - Retry/Cancel",
    {
        title: "Connection Failed",
        message: "Unable to connect to the server.\nWould you like to retry?",
        type: "error",
        buttons: "retry-cancel"
    }
);

// Test 7: Abort/Retry/Ignore
testDialog(
    "Error Dialog - Abort/Retry/Ignore",
    {
        title: "Critical Error",
        message: "A critical error occurred.\nChoose how to proceed.",
        type: "error",
        buttons: "abort-retry-ignore"
    }
);

console.log("");
console.log("=== Test Suite: Default Values ===");

// Test 8: Dialog with minimal options (should default to info/ok)
testDialog(
    "Minimal Dialog (defaults to info/ok)",
    {
        title: "Default Test",
        message: "This should show as an info dialog with OK button."
    }
);

// Test 9: Dialog with only message
testDialog(
    "Message Only (with default title behavior)",
    {
        title: "Message Only Test",
        message: "Testing with just a message and default type/buttons."
    }
);

console.log("");
console.log("=== Test Suite: Edge Cases ===");

// Test 10: Long message
testDialog(
    "Long Message Dialog",
    {
        title: "Long Content",
        message: "This is a very long message that tests how the dialog handles longer text content. " +
                 "It should wrap properly and display all the content. " +
                 "Lorem ipsum dolor sit amet, consectetur adipiscing elit. " +
                 "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
        type: "info",
        buttons: "ok"
    }
);

// Test 11: Unicode characters
testDialog(
    "Unicode Characters",
    {
        title: "Unicode Test 🎉",
        message: "Testing unicode: αβγδε 中文 русский العربية 🚀 ✓ ✗",
        type: "info",
        buttons: "ok"
    }
);

// Test 12: Empty strings (should use defaults)
testDialog(
    "Empty Type/Buttons",
    {
        title: "Empty Defaults",
        message: "Type and buttons are empty strings (should default to info/ok)",
        type: "",
        buttons: ""
    }
);

console.log("");
console.log("=== Test Suite: Case Insensitivity ===");

// Test 13: Mixed case type names
testDialog(
    "Mixed Case - Type 'Warning'",
    {
        title: "Case Test",
        message: "Testing case insensitivity for 'Warning' type",
        type: "Warning",
        buttons: "ok"
    }
);

// Test 14: Mixed case button names
testDialog(
    "Mixed Case - Buttons 'Yes-No'",
    {
        title: "Case Test",
        message: "Testing case insensitivity for 'Yes-No' buttons",
        type: "question",
        buttons: "Yes-No"
    }
);

console.log("");
console.log("=========================================");
console.log("   All MessageBox Tests Complete");
console.log("=========================================");
console.log("");
console.log("NOTE: This test requires manual interaction.");
console.log("The results shown are the buttons you clicked.");
console.log("");

// Exit after all tests
app.exit();
