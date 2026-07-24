alert("dashboard runnin");
// Path to the JSON "database" and the uploaded profile pictures.
// Adjust these if this file lives somewhere other than next to /database/.
const DATA_URL = "../database/data.json";
const UPLOADS_PATH = "../database/uploads/";

// In a real deployment the logged-in user would come from the session
// (e.g. an API endpoint or a cookie/token) rather than "the first user".
// Swap this out for however the backend identifies the current user.
function pickCurrentUser(users) {
  return users && users.length ? users[0] : null;
}

function initials(firstName, lastName) {
  const a = (firstName || "").trim()[0] || "";
  const b = (lastName || "").trim()[0] || "";
  return (a + b).toUpperCase() || "?";
}

function escapeHtml(str) {
  return String(str ?? "").replace(/[&<>"']/g, (c) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  }[c]));
}

function render(user) {
  const content = document.getElementById("content");

  const fullName = `${user.firstName || ""} ${user.lastName || ""}`.trim() || "Unknown user";
  const email = user.email || "—";
  const picUrl = user.profilePicture ? UPLOADS_PATH + encodeURIComponent(user.profilePicture) : null;

  content.className = "";
  content.innerHTML = `
    <div class="grid">
      <div class="card profile">
        ${
          picUrl
            ? `<img id="avatarPhoto" class="avatar-photo" src="${picUrl}" alt="${escapeHtml(fullName)}">`
            : `<div class="avatar-fallback">${initials(user.firstName, user.lastName)}</div>`
          }
        <p class="name">${escapeHtml(fullName)}</p>
        <p class="email">${escapeHtml(email)}</p>
        <span class="badge">Active</span>
      </div>

      <div class="card details">
        <h2>Account details</h2>
        <div class="rows">
          <div class="row"><span>First name</span><span>${escapeHtml(user.firstName || "—")}</span></div>
          <div class="row"><span>Last name</span><span>${escapeHtml(user.lastName || "—")}</span></div>
          <div class="row"><span>Email</span><span>${escapeHtml(email)}</span></div>
          <div class="row"><span>Session</span><span>Secure</span></div>
          <div class="row"><span>Profile picture</span><span>${user.profilePicture ? escapeHtml(user.profilePicture) : "None"}</span></div>
        </div>
      </div>
    </div>
  `;
  const img = document.getElementById("avatarPhoto");
  if (img) {
    img.addEventListener("error", () => {
      img.outerHTML = `<div class="avatar-fallback">${initials(user.firstName, user.lastName)}</div>`;
    });
  }
}

function renderError(message) {
  const content = document.getElementById("content");
  content.className = "state";
  content.textContent = message;
}

fetch(DATA_URL, { cache: "no-store" })
  .then((res) => {
    if (!res.ok) throw new Error("HTTP " + res.status);
    return res.json();
  })
  .then((data) => {
    const user = pickCurrentUser(data.users);
    if (!user) {
      renderError("No user data found.");
      return;
    }
    render(user);
  })
  .catch((err) => {
    console.error("Failed to load dashboard data:", err);
    renderError("Couldn't load account data. Please try logging in again.");
  });