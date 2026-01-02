/**
 * OpenSCAD AI - Web Interface
 * Main application JavaScript
 */

// ============================================================================
// Configuration & State
// ============================================================================

const CONFIG = {
    OPENAI_API_URL: 'https://api.openai.com/v1/chat/completions',
    DEFAULT_MODEL: 'gpt-4o',
    DEFAULT_WASM_PATH: '../build/openscad.js',
    STORAGE_KEYS: {
        API_KEY: 'openscad_ai_api_key',
        MODEL: 'openscad_ai_model',
        WASM_PATH: 'openscad_ai_wasm_path',
        CHAT_HISTORY: 'openscad_ai_chat_history'
    }
};

const SYSTEM_PROMPT = `You are an expert OpenSCAD programmer and 3D modeling assistant. Your task is to help users create 3D models by generating OpenSCAD code.

Guidelines:
1. Always respond with valid, working OpenSCAD code
2. Include helpful comments in the code explaining what each section does
3. Use parameterized designs when appropriate (variables at the top for easy customization)
4. Follow OpenSCAD best practices and conventions
5. If the user's request is unclear, ask clarifying questions
6. When generating code, wrap it in a code block with \`\`\`openscad and \`\`\`
7. Provide brief explanations of the design choices you made
8. Consider printability (wall thickness, overhangs, support requirements) when relevant

OpenSCAD Capabilities:
- 2D primitives: circle, square, polygon, text
- 3D primitives: cube, sphere, cylinder, polyhedron
- Transformations: translate, rotate, scale, mirror, multmatrix
- Boolean operations: union, difference, intersection
- Extrusions: linear_extrude, rotate_extrude
- Modules and functions for reusable code
- For loops and conditionals
- Mathematical operations

Always strive to create elegant, efficient, and well-documented code.`;

let state = {
    openscadInstance: null,
    isWasmLoaded: false,
    isGenerating: false,
    chatHistory: [],
    codeEditor: null,
    threeScene: null,
    threeRenderer: null,
    threeCamera: null,
    threeControls: null
};

// ============================================================================
// DOM Elements
// ============================================================================

const elements = {
    // Chat
    chatMessages: document.getElementById('chatMessages'),
    chatInput: document.getElementById('chatInput'),
    sendBtn: document.getElementById('sendBtn'),
    clearChatBtn: document.getElementById('clearChatBtn'),

    // Editor
    codeEditor: document.getElementById('codeEditor'),
    copyCodeBtn: document.getElementById('copyCodeBtn'),
    downloadBtn: document.getElementById('downloadBtn'),
    renderBtn: document.getElementById('renderBtn'),

    // Preview
    previewContainer: document.getElementById('previewContainer'),
    previewCanvas: document.getElementById('previewCanvas'),
    exportStlBtn: document.getElementById('exportStlBtn'),
    consoleOutput: document.getElementById('consoleOutput'),
    clearConsoleBtn: document.getElementById('clearConsoleBtn'),

    // Settings
    settingsBtn: document.getElementById('settingsBtn'),
    settingsModal: document.getElementById('settingsModal'),
    closeSettingsBtn: document.getElementById('closeSettingsBtn'),
    apiKeyInput: document.getElementById('apiKeyInput'),
    modelSelect: document.getElementById('modelSelect'),
    wasmPathInput: document.getElementById('wasmPathInput'),
    saveSettingsBtn: document.getElementById('saveSettingsBtn'),

    // Loading
    loadingOverlay: document.getElementById('loadingOverlay'),
    loadingText: document.getElementById('loadingText')
};

// ============================================================================
// Utility Functions
// ============================================================================

function showLoading(message = 'Loading...') {
    elements.loadingText.textContent = message;
    elements.loadingOverlay.classList.add('active');
}

function hideLoading() {
    elements.loadingOverlay.classList.remove('active');
}

function log(message, type = 'info') {
    const timestamp = new Date().toLocaleTimeString();
    const prefix = `[${timestamp}] `;
    const line = document.createElement('div');
    line.textContent = prefix + message;

    if (type === 'error') {
        line.classList.add('console-error');
    } else if (type === 'warning') {
        line.classList.add('console-warning');
    } else if (type === 'success') {
        line.classList.add('console-success');
    }

    elements.consoleOutput.appendChild(line);
    elements.consoleOutput.scrollTop = elements.consoleOutput.scrollHeight;
}

function clearConsole() {
    elements.consoleOutput.innerHTML = '';
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function parseMarkdown(text) {
    // Simple markdown parser for code blocks and basic formatting
    let html = escapeHtml(text);

    // Code blocks
    html = html.replace(/```(\w+)?\n([\s\S]*?)```/g, (match, lang, code) => {
        return `<pre><code class="language-${lang || ''}">${code.trim()}</code></pre>`;
    });

    // Inline code
    html = html.replace(/`([^`]+)`/g, '<code>$1</code>');

    // Bold
    html = html.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');

    // Italic
    html = html.replace(/\*([^*]+)\*/g, '<em>$1</em>');

    // Line breaks
    html = html.replace(/\n/g, '<br>');

    return html;
}

function extractOpenSCADCode(text) {
    // Extract code from markdown code blocks
    const codeBlockRegex = /```(?:openscad|scad)?\n([\s\S]*?)```/g;
    const matches = [];
    let match;

    while ((match = codeBlockRegex.exec(text)) !== null) {
        matches.push(match[1].trim());
    }

    return matches.length > 0 ? matches.join('\n\n') : null;
}

// ============================================================================
// Settings Management
// ============================================================================

function loadSettings() {
    const apiKey = localStorage.getItem(CONFIG.STORAGE_KEYS.API_KEY) || '';
    const model = localStorage.getItem(CONFIG.STORAGE_KEYS.MODEL) || CONFIG.DEFAULT_MODEL;
    const wasmPath = localStorage.getItem(CONFIG.STORAGE_KEYS.WASM_PATH) || CONFIG.DEFAULT_WASM_PATH;

    elements.apiKeyInput.value = apiKey;
    elements.modelSelect.value = model;
    elements.wasmPathInput.value = wasmPath;

    return { apiKey, model, wasmPath };
}

function saveSettings() {
    const apiKey = elements.apiKeyInput.value.trim();
    const model = elements.modelSelect.value;
    const wasmPath = elements.wasmPathInput.value.trim();

    localStorage.setItem(CONFIG.STORAGE_KEYS.API_KEY, apiKey);
    localStorage.setItem(CONFIG.STORAGE_KEYS.MODEL, model);
    localStorage.setItem(CONFIG.STORAGE_KEYS.WASM_PATH, wasmPath);

    elements.settingsModal.classList.remove('active');
    log('Settings saved successfully', 'success');

    return { apiKey, model, wasmPath };
}

function openSettings() {
    elements.settingsModal.classList.add('active');
}

function closeSettings() {
    elements.settingsModal.classList.remove('active');
}

// ============================================================================
// Chat Functions
// ============================================================================

function addMessage(content, role = 'assistant') {
    const messageDiv = document.createElement('div');
    messageDiv.className = `message ${role}`;

    const contentDiv = document.createElement('div');
    contentDiv.className = 'message-content';
    contentDiv.innerHTML = parseMarkdown(content);

    messageDiv.appendChild(contentDiv);
    elements.chatMessages.appendChild(messageDiv);
    elements.chatMessages.scrollTop = elements.chatMessages.scrollHeight;

    // Store in history
    state.chatHistory.push({ role, content });

    return messageDiv;
}

function addTypingIndicator() {
    const indicator = document.createElement('div');
    indicator.className = 'message assistant';
    indicator.id = 'typingIndicator';
    indicator.innerHTML = `
        <div class="typing-indicator">
            <span></span>
            <span></span>
            <span></span>
        </div>
    `;
    elements.chatMessages.appendChild(indicator);
    elements.chatMessages.scrollTop = elements.chatMessages.scrollHeight;
    return indicator;
}

function removeTypingIndicator() {
    const indicator = document.getElementById('typingIndicator');
    if (indicator) {
        indicator.remove();
    }
}

function clearChat() {
    // Keep only the initial welcome message
    const welcomeMessage = elements.chatMessages.querySelector('.message');
    elements.chatMessages.innerHTML = '';
    if (welcomeMessage) {
        elements.chatMessages.appendChild(welcomeMessage);
    }
    state.chatHistory = [];
    log('Chat cleared', 'info');
}

async function sendMessage() {
    const message = elements.chatInput.value.trim();
    if (!message || state.isGenerating) return;

    const settings = loadSettings();
    if (!settings.apiKey) {
        log('Please set your OpenAI API key in Settings', 'error');
        openSettings();
        return;
    }

    // Add user message
    addMessage(message, 'user');
    elements.chatInput.value = '';

    // Show typing indicator
    state.isGenerating = true;
    elements.sendBtn.disabled = true;
    const typingIndicator = addTypingIndicator();

    try {
        const response = await callOpenAI(message, settings);
        removeTypingIndicator();

        // Add assistant response
        addMessage(response, 'assistant');

        // Extract and update code if present
        const code = extractOpenSCADCode(response);
        if (code) {
            updateEditor(code);
            log('Code extracted and loaded into editor', 'success');
        }

    } catch (error) {
        removeTypingIndicator();
        log(`Error: ${error.message}`, 'error');
        addMessage(`Sorry, I encountered an error: ${error.message}. Please check your API key and try again.`, 'assistant');
    } finally {
        state.isGenerating = false;
        elements.sendBtn.disabled = false;
    }
}

async function callOpenAI(userMessage, settings) {
    const messages = [
        { role: 'system', content: SYSTEM_PROMPT },
        ...state.chatHistory.slice(-10).map(msg => ({
            role: msg.role === 'user' ? 'user' : 'assistant',
            content: msg.content
        })),
        { role: 'user', content: userMessage }
    ];

    const response = await fetch(CONFIG.OPENAI_API_URL, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${settings.apiKey}`
        },
        body: JSON.stringify({
            model: settings.model,
            messages: messages,
            temperature: 0.7,
            max_tokens: 4096
        })
    });

    if (!response.ok) {
        const error = await response.json().catch(() => ({}));
        throw new Error(error.error?.message || `API request failed: ${response.status}`);
    }

    const data = await response.json();
    return data.choices[0].message.content;
}

// ============================================================================
// Editor Functions
// ============================================================================

function initEditor() {
    // Initialize CodeMirror if available
    if (typeof CodeMirror !== 'undefined') {
        state.codeEditor = CodeMirror.fromTextArea(elements.codeEditor, {
            mode: 'text/x-csrc', // Use C-like mode for OpenSCAD
            theme: 'monokai',
            lineNumbers: true,
            indentUnit: 4,
            tabSize: 4,
            indentWithTabs: false,
            lineWrapping: true,
            matchBrackets: true,
            autoCloseBrackets: true
        });

        state.codeEditor.setSize('100%', '100%');
    }
}

function updateEditor(code) {
    if (state.codeEditor) {
        state.codeEditor.setValue(code);
    } else {
        elements.codeEditor.value = code;
    }
}

function getEditorContent() {
    if (state.codeEditor) {
        return state.codeEditor.getValue();
    }
    return elements.codeEditor.value;
}

function copyCode() {
    const code = getEditorContent();
    navigator.clipboard.writeText(code).then(() => {
        log('Code copied to clipboard', 'success');
    }).catch(err => {
        log('Failed to copy code: ' + err.message, 'error');
    });
}

function downloadCode() {
    const code = getEditorContent();
    const blob = new Blob([code], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'model.scad';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    log('File downloaded: model.scad', 'success');
}

// ============================================================================
// OpenSCAD WASM Functions
// ============================================================================

async function initOpenSCAD() {
    const settings = loadSettings();

    try {
        log('Loading OpenSCAD WASM module...', 'info');
        showLoading('Loading OpenSCAD...');

        // Dynamic import of OpenSCAD WASM module
        const OpenSCAD = (await import(settings.wasmPath)).default;

        state.openscadInstance = await OpenSCAD({ noInitialRun: true });

        // Set up fonts directory
        state.openscadInstance.FS.mkdir('/fonts');
        state.openscadInstance.FS.writeFile('/fonts/fonts.conf',
            `<?xml version="1.0" encoding="UTF-8"?>
            <!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
            <fontconfig></fontconfig>`);

        state.isWasmLoaded = true;
        log('OpenSCAD WASM loaded successfully', 'success');
        hideLoading();

    } catch (error) {
        log(`Failed to load OpenSCAD WASM: ${error.message}`, 'error');
        log('You can still use the AI assistant to generate code', 'warning');
        hideLoading();
    }
}

async function renderCode() {
    const code = getEditorContent();

    if (!code.trim()) {
        log('No code to render', 'warning');
        return;
    }

    if (!state.isWasmLoaded) {
        log('OpenSCAD WASM not loaded. Attempting to load...', 'warning');
        await initOpenSCAD();

        if (!state.isWasmLoaded) {
            log('Cannot render: OpenSCAD WASM failed to load', 'error');
            return;
        }
    }

    try {
        showLoading('Rendering model...');
        log('Starting render...', 'info');

        // Write the code to a file
        state.openscadInstance.FS.writeFile('input.scad', code);

        // Run OpenSCAD
        const exitCode = state.openscadInstance.callMain([
            'input.scad',
            '--backend=manifold',
            '-o', 'output.stl'
        ]);

        if (exitCode !== 0) {
            throw new Error(`OpenSCAD exited with code ${exitCode}`);
        }

        // Read the output STL
        const stlData = state.openscadInstance.FS.readFile('output.stl');

        log('Render complete!', 'success');

        // Display the STL in the preview
        displaySTL(stlData);

        hideLoading();

    } catch (error) {
        hideLoading();
        log(`Render error: ${error.message}`, 'error');
    }
}

function exportSTL() {
    if (!state.isWasmLoaded) {
        log('No rendered model to export', 'warning');
        return;
    }

    try {
        const stlData = state.openscadInstance.FS.readFile('output.stl');
        const blob = new Blob([stlData], { type: 'application/octet-stream' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = 'model.stl';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        log('STL file exported', 'success');
    } catch (error) {
        log(`Export error: ${error.message}`, 'error');
    }
}

// ============================================================================
// 3D Preview Functions (Three.js)
// ============================================================================

function initThreeJS() {
    const container = elements.previewContainer;
    const canvas = elements.previewCanvas;

    // Create scene
    state.threeScene = new THREE.Scene();
    state.threeScene.background = new THREE.Color(0x0f0f1a);

    // Create camera
    const aspect = container.clientWidth / container.clientHeight;
    state.threeCamera = new THREE.PerspectiveCamera(45, aspect, 0.1, 1000);
    state.threeCamera.position.set(50, 50, 50);

    // Create renderer
    state.threeRenderer = new THREE.WebGLRenderer({
        canvas: canvas,
        antialias: true
    });
    state.threeRenderer.setSize(container.clientWidth, container.clientHeight);
    state.threeRenderer.setPixelRatio(window.devicePixelRatio);

    // Add lights
    const ambientLight = new THREE.AmbientLight(0x404040, 0.5);
    state.threeScene.add(ambientLight);

    const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
    directionalLight.position.set(50, 50, 50);
    state.threeScene.add(directionalLight);

    const directionalLight2 = new THREE.DirectionalLight(0xffffff, 0.5);
    directionalLight2.position.set(-50, -50, -50);
    state.threeScene.add(directionalLight2);

    // Add grid helper
    const gridHelper = new THREE.GridHelper(100, 20, 0x444444, 0x222222);
    state.threeScene.add(gridHelper);

    // Add orbit controls
    if (typeof THREE.OrbitControls !== 'undefined') {
        state.threeControls = new THREE.OrbitControls(state.threeCamera, canvas);
        state.threeControls.enableDamping = true;
        state.threeControls.dampingFactor = 0.05;
    }

    // Animation loop
    function animate() {
        requestAnimationFrame(animate);
        if (state.threeControls) {
            state.threeControls.update();
        }
        state.threeRenderer.render(state.threeScene, state.threeCamera);
    }
    animate();

    // Handle resize
    window.addEventListener('resize', () => {
        const width = container.clientWidth;
        const height = container.clientHeight;
        state.threeCamera.aspect = width / height;
        state.threeCamera.updateProjectionMatrix();
        state.threeRenderer.setSize(width, height);
    });
}

function displaySTL(stlData) {
    // Show canvas, hide placeholder
    elements.previewCanvas.style.display = 'block';
    const placeholder = elements.previewContainer.querySelector('.preview-placeholder');
    if (placeholder) {
        placeholder.style.display = 'none';
    }

    // Initialize Three.js if not already done
    if (!state.threeScene) {
        initThreeJS();
    }

    // Remove existing model
    const existingModel = state.threeScene.getObjectByName('model');
    if (existingModel) {
        state.threeScene.remove(existingModel);
    }

    // Parse STL
    const loader = new THREE.STLLoader();
    let geometry;

    if (stlData instanceof Uint8Array) {
        geometry = loader.parse(stlData.buffer);
    } else {
        geometry = loader.parse(stlData);
    }

    // Center geometry
    geometry.computeBoundingBox();
    const center = new THREE.Vector3();
    geometry.boundingBox.getCenter(center);
    geometry.translate(-center.x, -center.y, -center.z);

    // Create mesh
    const material = new THREE.MeshPhongMaterial({
        color: 0x4CAF50,
        specular: 0x111111,
        shininess: 50,
        flatShading: false
    });

    const mesh = new THREE.Mesh(geometry, material);
    mesh.name = 'model';

    // Scale to fit view
    const box = new THREE.Box3().setFromObject(mesh);
    const size = box.getSize(new THREE.Vector3());
    const maxDim = Math.max(size.x, size.y, size.z);
    const scale = 30 / maxDim;
    mesh.scale.set(scale, scale, scale);

    state.threeScene.add(mesh);

    // Reset camera
    state.threeCamera.position.set(50, 50, 50);
    state.threeCamera.lookAt(0, 0, 0);
    if (state.threeControls) {
        state.threeControls.reset();
    }
}

// ============================================================================
// Event Listeners
// ============================================================================

function initEventListeners() {
    // Chat
    elements.sendBtn.addEventListener('click', sendMessage);
    elements.chatInput.addEventListener('keydown', (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            sendMessage();
        }
    });
    elements.clearChatBtn.addEventListener('click', clearChat);

    // Editor
    elements.copyCodeBtn.addEventListener('click', copyCode);
    elements.downloadBtn.addEventListener('click', downloadCode);
    elements.renderBtn.addEventListener('click', renderCode);

    // Preview
    elements.exportStlBtn.addEventListener('click', exportSTL);
    elements.clearConsoleBtn.addEventListener('click', clearConsole);

    // Settings
    elements.settingsBtn.addEventListener('click', openSettings);
    elements.closeSettingsBtn.addEventListener('click', closeSettings);
    elements.saveSettingsBtn.addEventListener('click', saveSettings);

    // Close modal on outside click
    elements.settingsModal.addEventListener('click', (e) => {
        if (e.target === elements.settingsModal) {
            closeSettings();
        }
    });

    // Keyboard shortcuts
    document.addEventListener('keydown', (e) => {
        // Ctrl/Cmd + Enter to render
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
            e.preventDefault();
            renderCode();
        }
        // Escape to close modal
        if (e.key === 'Escape') {
            closeSettings();
        }
    });
}

// ============================================================================
// Initialization
// ============================================================================

async function init() {
    log('OpenSCAD AI initialized', 'info');

    // Load settings
    loadSettings();

    // Initialize editor
    initEditor();

    // Initialize event listeners
    initEventListeners();

    // Try to load OpenSCAD WASM (optional - will work without it)
    // Uncomment the next line if you want to auto-load WASM on startup
    // await initOpenSCAD();

    log('Ready! Enter your API key in Settings to start generating 3D models.', 'info');
}

// Start the app
document.addEventListener('DOMContentLoaded', init);
