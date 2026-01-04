const canvas = document.querySelector("canvas");
const ctx = canvas.getContext("2d");

canvas.width = window.innerWidth;
canvas.height = window.innerHeight;

let frame = 0;

// Initialize target at center, but it will stay at the last mouse position thereafter
const target = {
  x: canvas.width / 2,
  y: canvas.height / 2,
  radius: 450,
};

window.addEventListener("mousemove", (e) => {
  target.x = e.x;
  target.y = e.y;
  console.log("-->", e.x, e.y);
});

class Particle {
  constructor(x, y) {
    this.x = x;
    this.y = y;
    this.baseX = x;
    this.baseY = y;
    this.lineLength = 5;
    this.alpha = 0;
  }

  draw() {
    if (this.alpha <= 0) return;
    ctx.beginPath();
    ctx.strokeStyle = `rgba(255, 255, 255, ${this.alpha})`;
    ctx.lineWidth = 1;
    ctx.moveTo(this.x, this.y);
    ctx.lineTo(this.x + this.lineLength, this.y + this.lineLength);
    ctx.stroke();
  }

  update(focusX, focusY) {
    // 1. Continuous Wavy Movement
    let waveFrequency = 0.006;
    let waveSpeed = 0.04;
    let waveAmplitude = 25;

    let waveOffset =
      Math.sin(this.baseX * waveFrequency + frame * waveSpeed) * waveAmplitude;
    let currentTargetY = this.baseY + waveOffset;

    // 2. Distance from the Target Point (Last Mouse Position)
    let dx = focusX - this.x;
    let dy = focusY - this.y;
    let distance = Math.sqrt(dx * dx + dy * dy);

    if (distance < target.radius) {
      // Smooth alpha based on proximity to the last known point
      let proximityAlpha = 1 - distance / target.radius;
      this.alpha = proximityAlpha;

      // Gentle interactive push
      const force = (target.radius - distance) / target.radius;
      this.x -= (dx / distance) * force * 10;
      this.y -= (dy / distance) * force * 10;
    } else {
      // Fade out lines outside the focal radius
      if (this.alpha > 0) this.alpha -= 0.02;
    }

    // 3. Constant drift back to their designated wave position
    this.x += (this.baseX - this.x) * 0.1;
    this.y += (currentTargetY - this.y) * 0.1;
  }
}

let particlesArray = [];
const spacing = 18;

function init() {
  particlesArray = [];
  for (let y = 0; y < canvas.height; y += spacing) {
    for (let x = 0; x < canvas.width; x += spacing) {
      particlesArray.push(new Particle(x, y));
    }
  }
}

function animate() {
  ctx.fillStyle = "black";
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  // We pass target.x and target.y directly.
  // Since we don't reset them on mouseout, they maintain the last position.
  particlesArray.forEach((particle) => {
    particle.update(target.x, target.y);
    particle.draw();
  });

  frame++;
  requestAnimationFrame(animate);
}

init();
animate();

window.addEventListener("resize", () => {
  canvas.width = window.innerWidth;
  canvas.height = window.innerHeight;
  // Optional: move target to new center if window is resized
  // and mouse hasn't moved yet.
  init();
});
