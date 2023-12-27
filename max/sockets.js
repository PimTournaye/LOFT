const maxApi = require('max-api');
const { io } = require("socket.io-client");

maxApi.addHandler("connect", (url) => {
  socket = io(url);
});

socket.on("connect", () => {
  maxApi.post("Connected to socket.io server");
});

socket.on("pigeon", (data) => {
  maxApi.outlet("pigeon", data);
})