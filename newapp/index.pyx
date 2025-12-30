
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

page("My about page", route="/about") {
        view("mydiv", style={
            "height": "50px",
            "background-color": "green"
        }, onclick=() {
            print("jsjj")
        }) {
            text('Click me and check console!')
        } 
    }

page("sjvjjkjk;", route="/sh") {
        view("mydiv", style={
            "height": "50px",
            "background-color": "green"
        }, onclick=() {
            print("jsjj")
        }) {
            text('Click me and check console!')
        } 
    }