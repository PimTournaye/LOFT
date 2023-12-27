import { Server } from "socket.io";

const io = new Server({ /* options */ });

io.on("connection", (socket) => {
  // ...
});


// Flocking parameters
const NUM_ENTITIES = 100;
const MAX_SPEED = 2;
const PERCEPTION_RADIUS = 50;
const SEPARATION_DISTANCE = 25;

class Vector2 {
  constructor(x, y) {
    this.x = x;
    this.y = y;
  }

  add(vector) {
    this.x += vector.x;
    this.y += vector.y;
  }

  subtract(vector) {
    this.x -= vector.x;
    this.y -= vector.y;
  }

  distanceTo(vector) {
    return Math.sqrt((this.x - vector.x) ** 2 + (this.y - vector.y) ** 2);
  }

  scale(length) {
    const currentLength = Math.sqrt(this.x * this.x + this.y * this.y);
    if (currentLength !== 0) {
      this.x *= length / currentLength;
      this.y *= length / currentLength;
    }
  }
}

class Entity {
  constructor(x, y) {
    this.position = new Vector2(x, y);
    this.velocity = new Vector2(Math.random() * 2 - 1, Math.random() * 2 - 1);
    this.velocity.scale(MAX_SPEED);
  }

  update(flock) {
    this.position.add(this.velocity);

    let averageVelocity = new Vector2(0, 0);
    let averagePosition = new Vector2(0, 0);
    let averageSeparation = new Vector2(0, 0);
    let numNeighbors = 0;

    for (let other of flock) {
      if (other === this) continue;

      let distance = this.position.distanceTo(other.position);

      if (distance < PERCEPTION_RADIUS) {
        averageVelocity.add(other.velocity);
        averagePosition.add(other.position);

        if (distance < SEPARATION_DISTANCE) {
          let diff = new Vector2(this.position.x - other.position.x, this.position.y - other.position.y);
          diff.scale(1 / distance);
          averageSeparation.add(diff);
        }

        numNeighbors++;
      }
    }

    if (numNeighbors > 0) {
      averageVelocity.scale(1 / numNeighbors);
      averagePosition.scale(1 / numNeighbors);
      averageSeparation.scale(1 / numNeighbors);

      this.velocity.add(averageVelocity.scale(0.02));
      this.velocity.add(averagePosition.scale(0.01));
      this.velocity.subtract(averageSeparation.scale(0.03));

      this.velocity.scale(MAX_SPEED);
    }
  }
}

const flock = [];
for (let i = 0; i < NUM_ENTITIES; i++) {
  flock.push(new Entity(Math.random() * 500, Math.random() * 500));
}

setInterval(() => {
  flock.forEach(entity => entity.update(flock));

  const trackedEntities = flock.slice(0, 3).map(entity => ({ x: entity.position.x, y: entity.position.y }));
  io.emit('flockUpdate', trackedEntities);
}, 1000 / 30);



io.listen(3000);