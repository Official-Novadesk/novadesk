
// var path = path.join('C:', 'Users', 'John', 'Desktop', 'widget.js');
var path = path.normalize("C:\\Users\\Documents\\..\\Desktop\\.\\widget.js");
win.addText({
  id: "myText",
  text: "Hello World!"
});

win.addImage({
  id: "myImage",
  path: "path/to/image.png"
});

novadesk.log(path)