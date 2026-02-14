var utils = require('../common/utils');

var weather_Widget = null;
var weather_Timer = null;
var current_Location = { lat: 29.688981, lon: 72.552545 }; 
var useFahrenheit = false; // Toggle for temperature units

function loadWeatherWidget() {
    if (weather_Widget) {
        return; // Widget already registered
    }

    weather_Widget = new widgetWindow({
        id: 'weather_Window',
        script: 'ui/ui.js',
        width: 212,
        height: 122,
        zPos: "ontop",
        show: !app.isFirstRun()
    })

    if (app.isFirstRun()) {
        weather_Widget.setProperties({
            x: ((system.getDisplayMetrics().primary.screenArea.width - 212) - 10),
            y: 10,
            show: true
        });
    }

    registerIPC();
}

function registerIPC() {
    function fetchWeather() {
        // Open-Meteo API endpoint for current weather
        var baseUrl = "https://api.open-meteo.com/v1/forecast";
        var url = baseUrl + "?latitude=" + current_Location.lat + "&longitude=" + current_Location.lon + "&current=temperature_2m,weather_code&timezone=auto";
        
        system.fetch(url, function(data) {
            if (data) {
                try {
                    var weatherData = JSON.parse(data);
                    var temperature = weatherData.current.temperature_2m;
                    
                    // Convert temperature based on preference
                    var displayTemp = useFahrenheit ? 
                        utils.celsiusToFahrenheit(temperature) : 
                        Math.round(temperature);
                    var tempUnit = useFahrenheit ? "F" : "C";
                    
                    var weatherCode = weatherData.current.weather_code;
                    
                    // Convert weather code to description
                    var weatherDesc = getWeatherDescription(weatherCode);
                    var weatherIcon = utils.getWeatherIcon(weatherCode);
                    
                    // Send to UI
                    ipc.send("weatherUpdate", {
                        temperature: displayTemp,
                        unit: tempUnit,
                        description: weatherDesc,
                        icon: weatherIcon
                    });
                } catch (e) {
                    console.log("Failed to parse weather data:", e);
                    ipc.send("weatherUpdate", {
                        temperature: "--",
                        description: "Error",
                        icon: "?"
                    });
                }
            } else {
                console.log("Failed to fetch weather data");
                ipc.send("weatherUpdate", {
                    temperature: "--",
                    description: "Offline",
                    icon: "?"
                });
            }
        });
    }

    fetchWeather();
    weather_Timer = setInterval(fetchWeather, 300000); // Update every 5 minutes
}

// IPC listener for UI requests
ipc.on("toggleTempUnit", function() {
    toggleTemperatureUnit();
});

function getWeatherDescription(code) {
    // Simplified weather code mapping (Open-Meteo codes)
    var descriptions = {
        0: "Clear sky",
        1: "Mainly clear",
        2: "Partly cloudy",
        3: "Overcast",
        45: "Fog",
        48: "Depositing rime fog",
        51: "Light drizzle",
        53: "Moderate drizzle",
        55: "Dense drizzle",
        61: "Slight rain",
        63: "Moderate rain",
        65: "Heavy rain",
        71: "Slight snow",
        73: "Moderate snow",
        75: "Heavy snow",
        80: "Slight rain showers",
        81: "Moderate rain showers",
        82: "Violent rain showers",
        95: "Thunderstorm",
        96: "Thunderstorm with hail",
        99: "Thunderstorm with heavy hail"
    };
    
    return descriptions[code] || "Unknown";
}

function unloadWeatherWidget() {
    if (weather_Timer) {
        clearInterval(weather_Timer);
        weather_Timer = null;
    }
    
    weather_Widget = null;
}

function toggleTemperatureUnit() {
    useFahrenheit = !useFahrenheit;
    // Force immediate update
    if (weather_Timer) {
        clearInterval(weather_Timer);
        registerIPC();
    }
}

module.exports = {
    loadWeatherWidget,
    unloadWeatherWidget,
    toggleTemperatureUnit
}