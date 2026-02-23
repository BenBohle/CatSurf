const CONFIG = {
    API_URL: '/cgi-bin/chat.py',
    AUTO_SCROLL_DELAY: 80,
    POLL_INTERVAL_MS: 3000,
};

const appState = {
    messages: [],
    currentUser: 'You',
    isLoading: false,
    isSending: false,
    pollTimer: null,
};

const elements = {
    messagesContainer: document.getElementById('messagesContainer'),
    messageForm: document.getElementById('messageForm'),
    messageInput: document.getElementById('messageInput'),
    sendButton: document.querySelector('.send-button'),
    loadingIndicator: document.getElementById('loadingIndicator'),
    typingIndicator: document.getElementById('typingIndicator'),
    userNameInput: document.getElementById('userNameInput'),
    nameDisplay: document.getElementById('nameDisplay'),
};

function formatTime(dateValue) {
    const date = new Date(dateValue);
    if (Number.isNaN(date.getTime())) {
        return 'just now';
    }

    const now = new Date();
    const diffMs = now - date;
    const diffMins = Math.floor(diffMs / 60000);
    const diffHours = Math.floor(diffMs / 3600000);

    if (diffMins < 1) return 'just now';
    if (diffMins < 60) return `${diffMins}m ago`;
    if (diffHours < 24) return `${diffHours}h ago`;

    return date.toLocaleDateString('en-US', {
        month: 'short',
        day: 'numeric',
        hour: '2-digit',
        minute: '2-digit',
    });
}

function normalizeMessage(raw) {
    const sender = typeof raw.sender === 'string' && raw.sender.trim()
        ? raw.sender.trim()
        : 'Anonymous';
    const text = typeof raw.text === 'string' ? raw.text : '';

    return {
        id: raw.id || `${sender}-${Date.now()}-${Math.random()}`,
        sender,
        text,
        timestamp: raw.timestamp || new Date().toISOString(),
        isOwnMessage: sender === appState.currentUser,
    };
}

function createMessageElement(message) {
    const messageDiv = document.createElement('div');
    messageDiv.className = `message ${message.isOwnMessage ? 'own' : 'other'}`;
    messageDiv.setAttribute('data-message-id', message.id);

    const senderElement = document.createElement('div');
    senderElement.className = 'message-sender';
    senderElement.textContent = message.sender;

    const bubbleWrapper = document.createElement('div');
    bubbleWrapper.className = 'message-bubble-wrapper';

    const bubble = document.createElement('div');
    bubble.className = 'message-bubble';
    bubble.textContent = message.text;

    const timeElement = document.createElement('div');
    timeElement.className = 'message-time';
    timeElement.textContent = formatTime(message.timestamp);

    bubbleWrapper.appendChild(bubble);
    messageDiv.appendChild(senderElement);
    messageDiv.appendChild(bubbleWrapper);
    messageDiv.appendChild(timeElement);
    return messageDiv;
}

function scrollToBottom() {
    setTimeout(() => {
        elements.messagesContainer.scrollTop = elements.messagesContainer.scrollHeight;
    }, CONFIG.AUTO_SCROLL_DELAY);
}

function showLoadingIndicator(show) {
    elements.loadingIndicator.style.display = show ? 'flex' : 'none';
}

function showTypingIndicator(show) {
    elements.typingIndicator.style.display = show ? 'flex' : 'none';
}

function setSendButtonDisabled(disabled) {
    elements.sendButton.disabled = disabled;
    elements.messageInput.disabled = appState.isSending;
}

function showErrorMessage(text) {
    const errorDiv = document.createElement('div');
    errorDiv.className = 'message other';

    const bubble = document.createElement('div');
    bubble.className = 'message-bubble';
    bubble.style.backgroundColor = '#E74C3C';
    bubble.style.color = '#FFFFFF';
    bubble.textContent = text;

    errorDiv.appendChild(bubble);
    elements.messagesContainer.appendChild(errorDiv);
    scrollToBottom();
}

function renderMessages() {
    const children = Array.from(elements.messagesContainer.children);
    children.forEach((child) => {
        if (!child.classList.contains('loading-indicator')) {
            child.remove();
        }
    });

    appState.messages.forEach((msg) => {
        elements.messagesContainer.appendChild(createMessageElement(msg));
    });

    scrollToBottom();
}

async function fetchMessages() {
    if (appState.isLoading) {
        return;
    }

    appState.isLoading = true;
    showLoadingIndicator(true);

    try {
        const response = await fetch(CONFIG.API_URL, {
            method: 'GET',
            headers: {
                Accept: 'application/json',
            },
            credentials: 'same-origin',
        });

        if (!response.ok) {
            throw new Error(`GET failed: ${response.status}`);
        }

        const payload = await response.json();
        const list = Array.isArray(payload) ? payload : (payload.messages || []);
        appState.messages = list.map(normalizeMessage);
        renderMessages();
    } catch (err) {
        console.error('fetchMessages error:', err);
        showErrorMessage('Nachrichten konnten nicht geladen werden.');
    } finally {
        appState.isLoading = false;
        showLoadingIndicator(false);
    }
}

async function sendMessage(text) {
    appState.isSending = true;
    setSendButtonDisabled(true);
    showTypingIndicator(true);

    try {
        const response = await fetch(CONFIG.API_URL, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                Accept: 'application/json',
            },
            credentials: 'same-origin',
            body: JSON.stringify({
                sender: appState.currentUser,
                text,
            }),
        });

        if (!response.ok) {
            throw new Error(`POST failed: ${response.status}`);
        }

        const payload = await response.json();
        const rawMessage = payload.message || payload;
        const msg = normalizeMessage(rawMessage);
        appState.messages.push(msg);
        renderMessages();
    } catch (err) {
        console.error('sendMessage error:', err);
        showErrorMessage('Nachricht konnte nicht gesendet werden.');
    } finally {
        appState.isSending = false;
        showTypingIndicator(false);
        const hasText = elements.messageInput.value.trim().length > 0;
        setSendButtonDisabled(!hasText);
    }
}

async function handleFormSubmit(event) {
    event.preventDefault();

    const text = elements.messageInput.value.trim();
    if (!text || appState.isSending) {
        return;
    }

    elements.messageInput.value = '';
    setSendButtonDisabled(true);
    await sendMessage(text);
    elements.messageInput.focus();
}

function handleInputChange() {
    const hasText = elements.messageInput.value.trim().length > 0;
    setSendButtonDisabled(!hasText || appState.isSending);
}

function handleNameChange() {
    const name = elements.userNameInput.value.trim();
    if (!name) {
        return;
    }

    appState.currentUser = name;
    elements.nameDisplay.textContent = name;
    localStorage.setItem('chatUserName', name);

    appState.messages = appState.messages.map((msg) => ({
        ...msg,
        isOwnMessage: msg.sender === appState.currentUser,
    }));
    renderMessages();
}

function startPolling() {
    if (appState.pollTimer) {
        clearInterval(appState.pollTimer);
    }

    appState.pollTimer = setInterval(() => {
        fetchMessages();
    }, CONFIG.POLL_INTERVAL_MS);
}

async function initializeApp() {
    const savedName = localStorage.getItem('chatUserName');
    if (savedName) {
        appState.currentUser = savedName;
        elements.userNameInput.value = savedName;
        elements.nameDisplay.textContent = savedName;
    }

    setSendButtonDisabled(true);
    elements.messageForm.addEventListener('submit', handleFormSubmit);
    elements.messageInput.addEventListener('input', handleInputChange);
    elements.userNameInput.addEventListener('change', handleNameChange);

    await fetchMessages();
    startPolling();
    elements.messageInput.focus();
}

if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeApp);
} else {
    initializeApp();
}
