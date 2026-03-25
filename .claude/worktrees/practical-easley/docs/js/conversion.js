/**
 * Wolf Pacc Audio — fires Meta ViewContent + GA4 view_item from #wpa-conversion JSON-LD block.
 * Place a <script type="application/json" id="wpa-conversion">...</script> before this script (defer).
 */
document.addEventListener('DOMContentLoaded', function () {
    var el = document.getElementById('wpa-conversion');
    if (!el) return;
    var cfg;
    try {
        cfg = JSON.parse(el.textContent);
    } catch (e) {
        return;
    }
    if (!cfg || !cfg.id || !cfg.name) return;

    var value = typeof cfg.value === 'number' ? cfg.value : parseFloat(cfg.value) || 0;
    var category = cfg.category || 'product';

    if (typeof fbq === 'function') {
        fbq('track', 'ViewContent', {
            content_name: cfg.name,
            content_ids: [String(cfg.id)],
            content_type: 'product',
            content_category: category,
            value: value,
            currency: 'USD'
        });
    }

    if (typeof gtag === 'function') {
        gtag('event', 'view_item', {
            currency: 'USD',
            value: value,
            items: [
                {
                    item_id: String(cfg.id),
                    item_name: cfg.name,
                    item_category: category,
                    price: value,
                    quantity: 1
                }
            ]
        });
    }
});
