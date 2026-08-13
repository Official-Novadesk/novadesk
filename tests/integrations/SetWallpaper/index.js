import { app } from "novadesk";
import * as system from "system";

console.log("=== SetWallpaper Integration (Latest API) ===");
// Using an online image URL instead of local path
const imagePath = "https://images.unsplash.com/photo-1557683316-973673baf926?w=1920&h=1080&fit=crop";
console.log("set(fill):", system.wallpaper.set(imagePath, "fill"));
console.log("set(stretch):", system.wallpaper.set(imagePath, "stretch"));
console.log("currentPath:", system.wallpaper.getCurrentPath());
app.exit();
