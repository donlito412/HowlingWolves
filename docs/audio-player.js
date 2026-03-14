/**
 * Wolf Pacc Audio — Reusable Waveform Player
 *
 * Usage: Add data-src="audio/file.wav" to any .wolf-player element.
 * Required inner structure:
 *   .wp-badge, .wp-title, .wp-current, .wp-total,
 *   .wp-play-btn, .wp-waveform > .wp-bars
 *
 * Dependencies: FontAwesome (already loaded globally)
 */
(function () {
    'use strict';

    const registry = []; // All player instances across the page

    // ─── Utilities ──────────────────────────────────────────────────
    function formatTime(s) {
        if (isNaN(s) || s < 0 || !isFinite(s)) return '0:00';
        const m = Math.floor(s / 60);
        const sec = Math.floor(s % 60);
        return `${m}:${sec.toString().padStart(2, '0')}`;
    }

    // Deterministic pseudo-random seeded from a string (for consistent bar shapes)
    function seededRand(str) {
        let seed = Array.from(str).reduce((a, c) => (a + c.charCodeAt(0)) | 0, 0);
        return function () {
            seed = (Math.imul(seed ^ (seed >>> 16), 0x45d9f3b) + 1013904223) | 0;
            return (seed >>> 0) / 0xffffffff;
        };
    }

    // ─── Waveform Bar Generation ─────────────────────────────────────
    function buildBars(barsEl, seed, count) {
        count = count || 72;
        const rand = seededRand(seed);
        const bars = [];
        for (let i = 0; i < count; i++) {
            const pos = i / (count - 1);
            // Natural envelope: slightly taller in middle, tapers at edges
            const envelope = 0.3 + 0.7 * Math.pow(Math.sin(pos * Math.PI), 0.5);
            const h = (0.2 + rand() * 0.8) * envelope;
            const bar = document.createElement('div');
            bar.className = 'wp-bar';
            bar.style.height = Math.round(h * 100) + '%';
            barsEl.appendChild(bar);
            bars.push(bar);
        }
        return bars;
    }

    // ─── Progress Update ─────────────────────────────────────────────
    function setProgress(bars, ratio) {
        const cutoff = Math.floor(ratio * bars.length);
        bars.forEach(function (bar, i) {
            bar.classList.toggle('wp-bar--played', i < cutoff);
        });
    }

    // ─── Player Init ─────────────────────────────────────────────────
    function initPlayer(container) {
        if (container._wpInit) return;
        container._wpInit = true;

        var src = container.dataset.src;
        if (!src) return;

        var playBtn    = container.querySelector('.wp-play-btn');
        var barsEl     = container.querySelector('.wp-bars');
        var waveformEl = container.querySelector('.wp-waveform');
        var currentEl  = container.querySelector('.wp-current');
        var totalEl    = container.querySelector('.wp-total');

        if (!playBtn || !barsEl || !waveformEl) return;

        // Build waveform bars
        var bars = buildBars(barsEl, src);
        var isPlaying = false;

        // Native audio element (no preload until play)
        var audio = new Audio();
        audio.preload = 'metadata';
        audio.src = src;

        audio.addEventListener('loadedmetadata', function () {
            if (totalEl) totalEl.textContent = formatTime(audio.duration);
        });

        audio.addEventListener('timeupdate', function () {
            if (currentEl) currentEl.textContent = formatTime(audio.currentTime);
            if (audio.duration) setProgress(bars, audio.currentTime / audio.duration);
        });

        audio.addEventListener('ended', function () {
            isPlaying = false;
            playBtn.innerHTML = '<i class="fas fa-play"></i>';
            container.classList.remove('wp-playing');
            setProgress(bars, 0);
            if (currentEl) currentEl.textContent = '0:00';
        });

        function pausePlayer() {
            audio.pause();
            isPlaying = false;
            playBtn.innerHTML = '<i class="fas fa-play"></i>';
            container.classList.remove('wp-playing');
        }

        function playPlayer() {
            // Stop any other playing instance
            registry.forEach(function (p) {
                if (p !== instance) p.pause();
            });
            audio.play().catch(function () {});
            isPlaying = true;
            playBtn.innerHTML = '<i class="fas fa-pause"></i>';
            container.classList.add('wp-playing');
        }

        playBtn.addEventListener('click', function () {
            if (isPlaying) {
                pausePlayer();
            } else {
                playPlayer();
            }
        });

        // Click waveform to seek
        waveformEl.addEventListener('click', function (e) {
            var rect = waveformEl.getBoundingClientRect();
            var ratio = Math.max(0, Math.min(1, (e.clientX - rect.left) / rect.width));
            if (audio.duration) {
                audio.currentTime = ratio * audio.duration;
                setProgress(bars, ratio);
                if (currentEl) currentEl.textContent = formatTime(audio.currentTime);
            }
        });

        var instance = {
            container: container,
            audio: audio,
            pause: pausePlayer
        };

        registry.push(instance);
        return instance;
    }

    // ─── Entry Point ─────────────────────────────────────────────────
    function init() {
        var containers = document.querySelectorAll('.wolf-player');
        if (!containers.length) return;

        // Lazy-init: create Audio objects when section scrolls near viewport
        var obs = new IntersectionObserver(function (entries) {
            entries.forEach(function (entry) {
                if (entry.isIntersecting) initPlayer(entry.target);
            });
        }, { rootMargin: '300px' });

        containers.forEach(function (c) { obs.observe(c); });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
