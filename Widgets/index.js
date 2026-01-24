// =====================
// Load Configuration
// =====================
var config = system.readJson("config.json");

if (!config) {
  console.log("Failed to load config.json, using defaults");
  config = {
    clock_widget_active: true,
    system_widget_active: true,
    network_widget_active: true,
    welcome_widget_active: true,
    is_first_launch: true
  };
}

var metrics = system.getDisplayMetrics();

// Global Widget References
var clockWindow = null;
var clockWindowProperties = null;

var systemWindow = null;
var systemWindowProperties = null;
var overallCPU = null;
var memory = null;

var networkWindow = null;
var networkWindowProperties = null;
var network = null;

var welcomeWindow = null;
var welcomeProps = null;


// =====================
// Helper Functions
// =====================

// Save current config to file
function saveConfig() {
  if (system.writeJson("config.json", config)) {
    console.log("Configuration saved successfully");
    return true;
  } else {
    console.log("Failed to save configuration");
    return false;
  }
}

// Update tray menu to reflect current widget states
function updateTrayMenu() {
  app.setTrayMenu([
    {
      text: "Widgets",
      items: [
        {
          text: "Clock Widget",
          checked: config.clock_widget_active ? true : false,
          action: function() {
            toggleWidget('clock');
          }
        },
        {
          text: "System Widget",
          checked: config.system_widget_active ? true : false,
          action: function() {
            toggleWidget('system');
          }
        },
        {
          text: "Network Widget",
          checked: config.network_widget_active ? true : false,
          action: function() {
            toggleWidget('network');
          }
        },
        {
          text: "Welcome Widget",
          checked: config.welcome_widget_active ? true : false,
          action: function() {
            toggleWidget('welcome');
          }
        }
      ]
    },
  ]);
}

// Toggle widget on/off
function toggleWidget(widgetName) {
  var configKey = widgetName + '_widget_active';
  var isActive = !config[configKey]; // Toggle
  
  config[configKey] = isActive;
  saveConfig();
  
  if (isActive) {
    // Create the widget
    if (widgetName === 'clock') createClockWidget();
    else if (widgetName === 'system') createSystemWidget();
    else if (widgetName === 'network') createNetworkWidget();
    else if (widgetName === 'welcome') createWelcomeWidget();
  } else {
    // Destroy the widget
    if (widgetName === 'clock') destroyClockWidget();
    else if (widgetName === 'system') destroySystemWidget();
    else if (widgetName === 'network') destroyNetworkWidget();
    else if (widgetName === 'welcome') destroyWelcomeWidget();
  }
  
  updateTrayMenu();
}


// =====================
// Widget Creation/Destruction Functions
// =====================

function createClockWidget() {
  if (clockWindow) return;

  clockWindow = new widgetWindow({
    id: "clock_Widget",
    script: "clock/clock.js",
    show: false
  });

  // Position Logic
  var defaultX = metrics.primary.screenArea.width - 210 - 20; // Approx width
  var defaultY = 20;

  if (config.is_first_launch) {
      clockWindow.setProperties({ x: defaultX, y: defaultY });
  }

  clockWindow.setContextMenu([
    {
      text: "Refresh",
      action: function() { clockWindow.refresh(); }
    },
    {
      text: "Close",
      action: function() {
        config.clock_widget_active = false;
        saveConfig();
        destroyClockWidget();
        updateTrayMenu();
      }
    }
  ]);

  clockWindow.setProperties({ show: true });
  clockWindowProperties = clockWindow.getProperties();
}

function destroyClockWidget() {
  if (clockWindow) {
    clockWindow.close();
    clockWindow = null;
    clockWindowProperties = null;
  }
}

function createSystemWidget() {
  if (systemWindow) return;

  systemWindow = new widgetWindow({
    id: "system_Widget",
    script: "system/system.js",
    show: false
  });

  overallCPU = new system.cpu();
  memory = new system.memory();

  // Position Logic
  var defaultX = metrics.primary.screenArea.width - 210 - 20;
  // Calculate Y based on clock if present, else default
  var prevProps = clockWindow ? clockWindow.getProperties() : null;
  var defaultY = prevProps ? (prevProps.y + prevProps.height + 20) : 20;

  if (config.is_first_launch) {
      systemWindow.setProperties({ x: defaultX, y: defaultY });
  }

  systemWindow.setContextMenu([
    {
      text: "Refresh",
      action: function() { systemWindow.refresh(); }
    },
    {
      text: "Close",
      action: function() {
        config.system_widget_active = false;
        saveConfig();
        destroySystemWidget();
        updateTrayMenu();
      }
    }
  ]);

  systemWindow.setProperties({ show: true });
  systemWindowProperties = systemWindow.getProperties();
}

function destroySystemWidget() {
  if (systemWindow) {
    systemWindow.close();
    systemWindow = null;
    systemWindowProperties = null;
    overallCPU = null;
    memory = null;
  }
}

function createNetworkWidget() {
  if (networkWindow) return;

  networkWindow = new widgetWindow({
    id: "network_Widget",
    script: "network/network.js",
    show: false
  });

  network = new system.network();

  // Position Logic
  var defaultX = metrics.primary.screenArea.width - 210 - 20;
  var defaultY = 20;
  if (systemWindow) {
      var p = systemWindow.getProperties();
      defaultY = p.y + p.height + 20;
  }

  if (config.is_first_launch) {
      networkWindow.setProperties({ x: defaultX, y: defaultY });
  }

  networkWindow.setContextMenu([
    {
      text: "Refresh",
      action: function() { networkWindow.refresh(); }
    },
    {
      text: "Close",
      action: function() {
        config.network_widget_active = false;
        saveConfig();
        destroyNetworkWidget();
        updateTrayMenu();
      }
    }
  ]);

  networkWindow.setProperties({ show: true });
  networkWindowProperties = networkWindow.getProperties();
}

function destroyNetworkWidget() {
  if (networkWindow) {
    networkWindow.close();
    networkWindow = null;
    networkWindowProperties = null;
    network = null;
  }
}

function createWelcomeWidget() {
  if (welcomeWindow) return;

  welcomeWindow = new widgetWindow({
    id: "welcome_Widget",
    script: "welcome/welcome.js",
    show: false
  });

  var tempProps = welcomeWindow.getProperties();
  
  var defaultX = (metrics.primary.screenArea.width - tempProps.width) / 2;
  var defaultY = (metrics.primary.screenArea.height - tempProps.height) / 2;

  if (config.is_first_launch) {
      welcomeWindow.setProperties({ x: defaultX, y: defaultY });
  }

  welcomeWindow.setContextMenu([
    {
      text: "Refresh",
      action: function() { welcomeWindow.refresh(); }
    },
    {
      text: "Close",
      action: function() {
        config.welcome_widget_active = false;
        saveConfig();
        destroyWelcomeWidget();
        updateTrayMenu();
      }
    }
  ]);

  welcomeWindow.setProperties({ show: true });
  welcomeProps = welcomeWindow.getProperties();
}

function destroyWelcomeWidget() {
  if (welcomeWindow) {
    welcomeWindow.close();
    welcomeWindow = null;
    welcomeProps = null;
  }
}


// =====================
// Initial Startup
// =====================

if (config.clock_widget_active) createClockWidget();
if (config.system_widget_active) createSystemWidget();
if (config.network_widget_active) createNetworkWidget();
if (config.welcome_widget_active) createWelcomeWidget();


// =====================
// First Launch Post-Setup
// =====================
if (config.is_first_launch) {
  config.is_first_launch = false;
  saveConfig();
}

// =====================
// Initialize Tray Menu
// =====================
updateTrayMenu();

// =====================
// Global Management Loop
// =====================
setInterval(function () {
  ipc.send('update_Elements');

  if (overallCPU && memory) {
    var stats = overallCPU.usage();
    var memStats = memory.stats();

    ipc.send('cpu-usage', stats);
    ipc.send('memory-usage', memStats.percent);
  }

  if (network) {
    var netStats = network.stats();

    // Convert bytes to KB/s for display
    var netInKB = (netStats.netIn / 1024).toFixed(2);
    var netOutKB = (netStats.netOut / 1024).toFixed(2);

    ipc.send('net-in', netInKB);
    ipc.send('net-out', netOutKB);
  }

}, 1000);


ipc.on('executeURL', function (url) {
  system.execute(url);
});