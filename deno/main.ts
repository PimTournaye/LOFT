import { serve } from "https://deno.land/std@0.150.0/http/server.ts";
import { Server } from "https://deno.land/x/socket_io@0.2.0/mod.ts";
import {
  Bson,
  MongoClient,
} from "https://deno.land/x/mongo@v0.32.0/mod.ts";

////////////////////
////// MongoDB /////
////////////////////

const client = new MongoClient();
const collection = "Dakar-Tests"

// Connecting to a Mongo Atlas Database
await client.connect({
  db: "LOFT",
  tls: true,
  servers: [
    {
      host: Deno.env.get("MONGODB_URL"),
      port: 27017,
    },
  ],
  credential: {
    username: Deno.env.get("MONGODB_USERNAME"),
    password: Deno.env.get("MONGODB_PASSWORD"),
    db: "LOFT",
    mechanism: "SCRAM-SHA-1",
  },
});

////////////////////
//// Socket.io /////
////////////////////
const io = new Server();

io.on("connection", (socket) => {
  console.log(`socket ${socket.id} connected`);

  socket.emit("hello", "world");

  socket.on("disconnect", (reason) => {
    console.log(`socket ${socket.id} disconnected due to ${reason}`);
  });
});

await serve(io.handler(), {
  port: 3000,
});