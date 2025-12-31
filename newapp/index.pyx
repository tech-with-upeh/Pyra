page("My APP", style=#{
    "height": "100%",
    "width": "100%",
    "padding": "0",
    margin: "0",
    "background-color": "pink"
}) {
    pi = 3142
    @state num : "red"    
    view("mydiv", style={
    "height": "50px",
    "background-color": "green",
    "padding": "0"
}, onclick=(num) {
        print(num)
    }) {
        text('Clime and check console!')
    }

    view("myview", style={
        "height": "100px",
        "background-color": num
    }, onclick=(num) {
        if(num == "red") {
            num = "yellow"
        } else {
            num = "red"
        }
        print("printing: ->" + num)
    })
}

page("Oh Oh hhhhh", route="notfound") {
    text("My custidwjbdsbkjj")
}

page("About Page", route="about") {
    text("My About")
}