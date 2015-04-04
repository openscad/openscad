function addSubmenu(menu, name, files) {
    var ul = document.createElement('ul');
    ul.appendChild(document.createElement('h3'));
    for (var a = 0; a < files.length; a++) {
        var menulink = document.createElement('a');
        menulink.href = "../" + name + "/" + files[a] + ".html";
        ul.appendChild(menulink);
        var li = document.createElement('li');
        li.appendChild(document.createTextNode(files[a]));
        menulink.appendChild(li);
    }

    menu.appendChild(ul);
}

function addMenu() {
    var menudiv = document.getElementById("menu");
    var menu = document.createElement('ul');
    menudiv.appendChild(menu);

    for (var a = 0; a < openscad_examples.length; a++) {
        var entry = document.createElement('li');
        var menulink = document.createElement('a');
        menulink.href = "#";
        entry.appendChild(menulink);
        var entrydiv = document.createElement('div');
        menulink.appendChild(entrydiv);
        menulink.appendChild(document.createTextNode(openscad_examples[a].name));

        addSubmenu(entry, openscad_examples[a].name, openscad_examples[a].files);

        menu.appendChild(entry);
    }
}

function load() {
    addMenu();
}
