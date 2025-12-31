
// Test Byte Formatting
novadesk.log("--- Byte Formatting ---");
novadesk.log("1500 bytes:", novadesk.units.formatBytes(1500));
novadesk.log("1500 bytes (0 decimals):", novadesk.units.formatBytes(1500, 0));
novadesk.log("1024 bytes:", novadesk.units.formatBytes(1024));
novadesk.log("0 bytes:", novadesk.units.formatBytes(0));
novadesk.log("123456789 bytes:", novadesk.units.formatBytes(123456789));

// Test Time Formatting
novadesk.log("--- Time Formatting ---");
novadesk.log("3665 seconds:", novadesk.units.formatSeconds(3665));
novadesk.log("65 seconds:", novadesk.units.formatSeconds(65));
novadesk.log("0 seconds:", novadesk.units.formatSeconds(0));
novadesk.log("3600 seconds:", novadesk.units.formatSeconds(3600));

// Test Temperature Conversion
novadesk.log("--- Temperature Conversion ---");
novadesk.log("100 C to F:", novadesk.units.convertTemperature(100, "C", "F"));
novadesk.log("0 C to F:", novadesk.units.convertTemperature(0, "C", "F"));
novadesk.log("212 F to C:", novadesk.units.convertTemperature(212, "F", "C"));
novadesk.log("0 K to C:", novadesk.units.convertTemperature(0, "K", "C"));
novadesk.log("100 C to K:", novadesk.units.convertTemperature(100, "C", "K"));

// Test Percentage Formatting
novadesk.log("--- Percentage Formatting ---");
novadesk.log("0.5 ->", novadesk.units.formatPercentage(0.5));
novadesk.log("0.55 ->", novadesk.units.formatPercentage(0.55));
novadesk.log("1.0 ->", novadesk.units.formatPercentage(1.0));
novadesk.log("0.0 ->", novadesk.units.formatPercentage(0.0));
novadesk.log("0.123 ->", novadesk.units.formatPercentage(0.123));
novadesk.log("0.123 (1 dec) ->", novadesk.units.formatPercentage(0.123, 1));

// Test Normalization
novadesk.log("--- Normalization ---");
novadesk.log("Progress(50, 100):", novadesk.units.calculateProgress(50, 100));
novadesk.log("Progress(200, 100):", novadesk.units.calculateProgress(200, 100));

novadesk.log("Parse '50%':", novadesk.units.parsePercentage("50%"));
novadesk.log("Parse 75:", novadesk.units.parsePercentage(75));
novadesk.log("Parse 0.2:", novadesk.units.parsePercentage(0.2));

widgetWindow(500, 500, "Unit Conversion Tests");
win.show();
