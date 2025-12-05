/**
 * @file server.js
 * @brief Node.js Backend for BeagleY-AI Smart Doorbell
 * * This server acts as the bridge between the C application running on the BeagleY-AI
 * and the outside world (Discord). It listens for UDP messages from the C app
 * and forwards alerts to a Discord channel via Webhooks.
 * * Key Functions:
 * 1. UDP Listener: Receives simple text alerts from the C app (e.g., "Motion Detected").
 * 2. Image Handling: Reads the snapshot image saved by the C app from the local filesystem.
 * 3. Discord Integration: Constructs a multipart POST request to send both the alert text
 * and the image file to Discord.
 */

const dgram = require('dgram');
const axios = require('axios');
const FormData = require('form-data');
const fs = require('fs');

// --- CONFIGURATION ---
// Discord Webhook URL: Where alerts are sent.
// Ideally, this should be an environment variable for security.
const DISCORD_WEBHOOK_URL = 'https://discord.com/api/webhooks/1440872654952206497/Fu1Saq9C2B4zQVPHxPcbDMHsbWeee5cvLWW3Iy5nIXZyZr3qLRv_EC3geUjMqHGT-91K';

// ESP32 Capture URL: Used for debugging or direct capture if needed (though the C app handles capture now)
// Most standard ESP32 firmware uses /capture for high-res stills
const ESP32_CAPTURE_URL = 'http://192.168.4.1/still'; 

// UDP Server Settings: Must match the C app's target port and address
const UDP_SERVER_PORT = 7070;
const UDP_SERVER_ADDRESS = '127.0.0.1';

// Path where the C app saves the image before sending the UDP alert
const LOCAL_IMAGE_PATH = '/tmp/visitor.jpg';

// --- DISCORD FUNCTION WITH IMAGE ---
/**
 * @brief Sends an alert message with an image attachment to Discord.
 * @param {string} message The alert text received from the C app.
 */
const sendDiscordAlert = async (message) => {
    console.log(`🚨 Alert Received: "${message}". Fetching camera snapshot...`);

    // --- Dynamic Title Logic ---
    let alertTitle = "🚨 Security Alert";
    const msgLower = message.toLowerCase();

    if (msgLower.includes("motion")) {
        alertTitle = "📸 Motion Detected";
    } else if (msgLower.includes("tamper")) {
        alertTitle = "⚠️ Tamper Alert!";
    } else if (msgLower.includes("unlocked")) {
        alertTitle = "🔓 Door Unlocked";
    } else if (msgLower.includes("button") || msgLower.includes("pressed")) {
        alertTitle = "🔔 Doorbell Ring";
    }

    try {
        // 1. Check if the snapshot file exists
        // The C app is responsible for saving 'visitor.jpg' to /tmp before sending the UDP packet.
        if (!fs.existsSync(LOCAL_IMAGE_PATH)) {
            console.error('❌ Snapshot file not found at ' + LOCAL_IMAGE_PATH);
            // If image is missing, send a text-only alert so the user is still notified.
            sendTextOnlyFallback(message, "Snapshot file missing");
            return;
        }

        // 2. Read the file from disk
        const imageStream = fs.createReadStream(LOCAL_IMAGE_PATH);

        // 3. Prepare the Form Data for Discord
        // Discord Webhooks require multipart/form-data when uploading files.
        const form = new FormData();
        
        // Attach the image file stream
        form.append('file', imageStream, 'snapshot.jpg');
        
        // Attach the JSON payload for the message content (embeds, text, etc.)
        const payload = {
            username: 'BeagleY Security',
            embeds: [{
                title: alertTitle, // Uses the dynamic title calculated above
                description: message,
                color: msgLower.includes("unlocked") ? 5763719 : 15158332, // Green for unlock, Red for others
                image: {
                    url: "attachment://snapshot.jpg" // References the 'file' part we attached above
                },
                footer: { text: `BeagleY-AI • ${new Date().toLocaleTimeString()}` }
            }]
        };

        // 'payload_json' is the specific field Discord expects for the JSON part of a multipart request
        form.append('payload_json', JSON.stringify(payload));

        // 4. Send the POST request to Discord
        await axios.post(DISCORD_WEBHOOK_URL, form, {
            headers: {
                ...form.getHeaders() // Crucial: Adds the correct Content-Type with boundary
            }
        });

        console.log('✅ Snapshot sent to Discord!');

    } catch (error) {
        console.error('❌ Error:', error.message);
        // If the request fails (e.g., network issue), try to send a fallback text message
        if (error.code === 'ECONNREFUSED' || error.code === 'ETIMEDOUT') {
            sendTextOnlyFallback(message, "Camera unreachable");
        }
    }
};

/**
 * @brief Sends a text-only alert if the image upload fails.
 * @param {string} message The original alert message.
 * @param {string} errorDetail Reason why the image failed.
 */
const sendTextOnlyFallback = async (message, errorDetail) => {
    try {
        await axios.post(DISCORD_WEBHOOK_URL, {
            content: `⚠️ **Alert:** ${message}\n(Camera snapshot failed: ${errorDetail})`
        });
    } catch (e) { console.error("Failed to send fallback message"); }
};

// --- UDP SERVER SETUP ---
// Creates a socket to listen for incoming UDP packets from the C app
const server = dgram.createSocket('udp4');

/**
 * @brief Handles incoming UDP messages.
 * This is the trigger point. When the C app detects motion/RFID/Button,
 * it sends a string here, which triggers the Discord alert logic.
 */
server.on('message', (msg, rinfo) => {
    const receivedMessage = msg.toString().trim();
    console.log(`UDP Trigger from ${rinfo.address}: ${receivedMessage}`);
    
    // Trigger the alert workflow
    sendDiscordAlert(receivedMessage);
});

/**
 * @brief Callback when the server starts listening.
 */
server.on('listening', () => {
    const address = server.address();
    console.log(`📡 Security System Listening on ${address.port}`);
    console.log(`📷 Camera Target: ${ESP32_CAPTURE_URL}`);
});

// Bind to localhost port 7070
server.bind(UDP_SERVER_PORT, UDP_SERVER_ADDRESS);