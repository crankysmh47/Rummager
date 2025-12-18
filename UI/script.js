document.addEventListener('DOMContentLoaded', () => {
    // API Configuration
    const API_URL = 'http://127.0.0.1:5000/search';

    // Elements
    const wrapper = document.getElementById('app-wrapper');
    const searchForm = document.getElementById('search-form');
    const searchInput = document.getElementById('search-input');
    const clearBtn = document.getElementById('clear-btn');
    const resultsArea = document.getElementById('results-area');
    const resultsList = document.getElementById('results-list');
    const statsBar = document.getElementById('stats-bar');
    const loader = document.getElementById('loader');

    // UI: Handle Input Clear Button
    searchInput.addEventListener('input', () => {
        clearBtn.classList.toggle('hidden', searchInput.value.trim() === '');
    });

    clearBtn.addEventListener('click', () => {
        searchInput.value = '';
        searchInput.focus();
        clearBtn.classList.add('hidden');
        // Optional: Reset view to center if cleared?
        // wrapper.classList.remove('has-searched');
        // resultsArea.classList.add('hidden');
    });

    // Core Search Logic
    searchForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const query = searchInput.value.trim();
        if (!query) return;

        // 1. Trigger Animation (Move Top)
        wrapper.classList.add('has-searched');
        resultsArea.classList.remove('hidden');
        
        // 2. Show Loader, Hide Old Results
        loader.classList.remove('hidden');
        resultsList.innerHTML = '';
        statsBar.textContent = '';

        try {
            const startTime = performance.now();
            
            // 3. Fetch Data
            const response = await fetch(`${API_URL}?q=${encodeURIComponent(query)}`);
            if (!response.ok) throw new Error('Backend error');
            const data = await response.json();
            
            const endTime = performance.now();
            const timeTaken = data.time_taken || ((endTime - startTime)/1000).toFixed(3);

            // 4. Render
            renderStats(data.total_results || (data.results ? data.results.length : 0), timeTaken);
            renderResults(data.results || []);

        } catch (error) {
            console.error(error);
            resultsList.innerHTML = `<div style="color:#d93025; padding: 20px;">Error loading results. Is the server running?</div>`;
        } finally {
            loader.classList.add('hidden');
        }
    });

    function renderStats(count, time) {
        statsBar.innerHTML = `About ${count} results <nobr>(${time} seconds)</nobr>`;
    }

    function renderResults(results) {
        if (results.length === 0) {
            resultsList.innerHTML = '<p style="padding-top:20px">No documents found.</p>';
            return;
        }

        const html = results.map(doc => {
            // Safety escape
            const safe = (str) => (str || '').replace(/</g, "&lt;").replace(/>/g, "&gt;");
            
            return `
                <div class="result-card">
                    <div class="result-title">
                        <a href="${doc.url || '#'}" target="_blank">${safe(doc.title)}</a>
                    </div>
                    <div class="result-meta">
                        ${safe(Array.isArray(doc.authors) ? doc.authors.join(', ') : doc.authors)}
                    </div>
                    <div class="result-snippet">
                        ${safe(doc.abstract)}
                    </div>
                </div>
            `;
        }).join('');

        resultsList.innerHTML = html;
    }
});