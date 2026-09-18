## Episode 1 - Sentinel Design Review
Goal:
    Understanding the tradeoffs of using a sentinel node instead of NULL before implementing balancing options

Prompt:
    "Can you explain in depth the benefit to using a sentinel instead of NUll and what issues might arise with this approach"

Response summary:
    Claude explaine that the sentinel pattern simplifies red-black tree algorithms by allowing leaf nodes to be represented by a real node instead of NULL. This helped reduce a special case but also identified several risks including mixing of NIL and NULL.

Judgment
    Accepted

Reasoning
    The explanation improved my understanding of why the provided codebase uses a sentinel and how futre balancing code would rely on that design.