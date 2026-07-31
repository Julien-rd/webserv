const form = document.getElementById("registerForm");
const preview = document.getElementById("preview");
const fileInput = document.getElementById("profilePicture");
const status = document.getElementById("status");
const submitBtn = document.getElementById("submitBtn");

//
// preview
//
fileInput.addEventListener("change", () => {

    const file = fileInput.files[0];

    if(!file){
        preview.style.backgroundImage = "";
        preview.textContent = "Kein Bild";
        return;
    }

    const reader = new FileReader();

    reader.onload = e => {
        preview.style.backgroundImage =
            `url(${e.target.result})`;
        preview.textContent = "";
    };

    reader.readAsDataURL(file);

});

//
// submit
//
form.addEventListener("submit", async (e)=>{
  
    e.preventDefault();

    status.textContent = "";
    status.className = "status";

    if(!form.reportValidity())
        return;

    submitBtn.disabled = true;
    submitBtn.textContent = "Wird gesendet...";

    try{

        //--------------------------------------------------
        // STEP 1
        // Upload image as RAW BODY
        //--------------------------------------------------

        let avatarFilename = "";

        const file = fileInput.files[0];
        
        if(file){
            const uploadResponse = await fetch("/registration/upload.py",{

                method:"POST",

                headers:{
                    "Content-Type":file.type
                },

                body:file

            });

            if(!uploadResponse.ok)
                throw new Error("Bild konnte nicht hochgeladen werden.");

            avatarFilename = await uploadResponse.text();

            avatarFilename = avatarFilename.trim();

        }

        //--------------------------------------------------
        // STEP 2
        // Register user (POST, url-encoded body — same
        // convention /login.py read via stdin;
        // a GET request has no body, so query-string params
        // never reached 's CONTENT_LENGTH read)
        //--------------------------------------------------

        const params = new URLSearchParams({

            firstName:document.getElementById("firstName").value,
            lastName:document.getElementById("lastName").value,
            email:document.getElementById("email").value,
            password:document.getElementById("password").value,
            avatar:avatarFilename

        });

        const response = await fetch(

            "/registration/register.py",

            {
                method:"POST",
                headers:{
                    "Content-Type":"application/x-www-form-urlencoded"
                },
                body:params.toString()
            }

        );

        const text = await response.text();

        if(!response.ok)
            throw new Error(text || "Registrierung fehlgeschlagen.");

        status.textContent = text;
        status.classList.add("ok");

        form.reset();

        preview.style.backgroundImage = "";
        preview.textContent = "Kein Bild";

        setTimeout(() => {
            window.location.href = "../login/";
        }, 900);

    }
    catch(err){

        status.textContent = err.message;
        status.classList.add("error");

    }

    submitBtn.disabled = false;
    submitBtn.textContent = "Registrieren";

});

