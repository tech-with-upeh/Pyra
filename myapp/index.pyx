
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
