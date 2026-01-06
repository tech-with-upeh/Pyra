const ws = new WebSocket("ws://localhost:9000");

ws.onopen = () => {
  console.log("Connected!");
  ws.send("Hello server!");
};

ws.onmessage = (event) => {
  console.log("Received from server:", event.data);
};
