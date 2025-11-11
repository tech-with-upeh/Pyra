#include <emscripten.h>

// Define a C function that maps to JavaScript code
EM_JS(void, js_redirect, (const char* url_ptr), {
    // Inside this block is raw JavaScript
    // url_ptr is an integer (pointer) passed from C++
    // Use Emscripten utility to convert C string to JS string
    // const url = UTF8ToString(url_ptr);
    // window.location.href = url;
     const p1 = document.createElement("div");
    p1.innerHTML = '<h1>index Page Content</h1>';
     p1.onclick = navigateTo("/home", "home", displayHomePage);
    document.body.appendChild(p1);
   

    // This function handles the URL change without a full page reload.
function navigateTo(url, title, contentFunction) {
    // 1. Update the URL in the browser's address bar
    window.history.pushState({ page: url }, title, url);

    // 2. Call the function that updates your page content/view
    contentFunction();
}

// A function that would display your "home" content
function displayHomePage() {
    console.log("Welcome to the Home Page!");
    // In a real application, you'd update DOM elements here
    const p1 = document.createElement("div");
    document.body.appendChild(p1);
    p1.innerHTML = '<h1>Home Page Content</h1>';
    // You could potentially call a Wasm function here too
}
});

// Example usage:
int main() {
    // ... some C++ logic ...
    js_redirect("https://www.example.com/new_page");
    return 0;
}
