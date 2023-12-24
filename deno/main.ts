import { serve } from "https://deno.land/std@0.150.0/http/server.ts";
import { Server } from "https://deno.land/x/socket_io@0.2.0/mod.ts";
// Environment Variables
import { load } from "https://deno.land/std@0.210.0/dotenv/mod.ts";
const env = await load();

////////////////////
////// MongoDB /////
////////////////////
import {
  MongoClient,
  ObjectId,
} from "https://deno.land/x/atlas_sdk@v1.1.2/mod.ts";

const client = new MongoClient({
  endpoint: "https://data.mongodb-api.com/app/application-0-zsavc/endpoint/data/v1",
  dataSource: "Cluster0",
  auth: {
    apiKey: Deno.env.get("MONGODB_API_KEY"),
  },
});

const db = client.database("LOFT");
const test = db.collection("Dakar-Tests");


////////////////////
//// Socket.io /////
////////////////////
const io = new Server();

io.on("connection", (socket) => {
  console.log(`socket ${socket.id} connected`);

  socket.emit("hello", "world");

  socket.on("pigeon", async (data) => {
    console.log(data);

    //  Push data to MongoDB
    // Insert a document with the current timestamp
    // const insertId = await test.insertOne({
    //   data,
    //   createdAt: new Date(),
    // });
    
  });

  socket.on("disconnect", (reason) => {
    console.log(`socket ${socket.id} disconnected due to ${reason}`);
  });
});

await serve(io.handler(), {
  port: 3000,
});