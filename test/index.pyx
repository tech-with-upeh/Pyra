stylesheet {
    btn = {
        style: {
            padding: "10px",
            color: "white"
        },
        media: {
            "(max-width: 600px)": { padding: "5px" },
            "(max-width: 400px)": { padding: "2px" }
        }
    },
    card = { style: { margin: "10px" } }
}


@state num : 0
    page("My APP") {
        view("mydiv", style={
            "height": "50px",
            "background-color": "green"
        }, onclick=(num) {
            num = num + 1
            print(num)
        }) {
            text('Click me and check console!')
        } 
    }
