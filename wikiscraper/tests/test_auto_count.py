import unittest
from unittest.mock import MagicMock, patch, call
from src.wiki_scraper import WikiScraper, ArticleNotFound


class TestAutoCount(unittest.TestCase):
    def setUp(self):
        self.scraper = WikiScraper()
        self.scraper.processor = MagicMock()
        # Mock file operations to prevent IO
        self.scraper._update_word_counts_file = MagicMock()

    @patch('time.sleep')
    def test_bfs_traversal(self, mock_sleep):
        # Setup mock behavior
        # Page A links to B and C
        # Page B links to D
        # Depth = 1, so D should NOT be visited

        def fetch_side_effect(term):
            if term == "Start": return "<html>Start</html>"
            if term == "B": return "<html>B</html>"
            if term == "C": return "<html>C</html>"
            if term == "D": return "<html>D</html>"
            raise ArticleNotFound("Not found")

        def links_side_effect(html):
            if html == "<html>Start</html>": return ["B", "C"]
            if html == "<html>B</html>": return ["D"]
            return []

        self.scraper.processor.fetch_article.side_effect = fetch_side_effect
        self.scraper.processor.get_links.side_effect = links_side_effect
        self.scraper.processor.count_words.return_value = {"word": 1}

        # Run auto-count with depth 1
        self.scraper.handle_auto_count("Start", depth=1, wait=0.1)

        # Verify calls
        # Should fetch Start, B, C. D is depth 2, so should be queued?
        # Wait: Depth 1 means 1 edge from start.
        # Start (depth 0) -> links to B(1), C(1).
        # We process Start. Links B, C added to queue with depth 1.
        # We process B. Depth is 1. Since current_depth (1) < depth (1) is False, we do NOT get links from B.
        # We process C. Depth 1. No links.
        # D is never added.

        processed = [args[0] for args, _ in self.scraper.processor.fetch_article.call_args_list]
        self.assertIn("Start", processed)
        self.assertIn("B", processed)
        self.assertIn("C", processed)
        self.assertNotIn("D", processed)

        # Verify update calls
        self.assertEqual(self.scraper._update_word_counts_file.call_count, 3)

        # Verify wait calls (should be called 2 times: after Start, after B. C is last, might strictly be called or not depending on implementation)
        # My impl calls wait if queue is not empty.
        # Queue: [Start] -> pop -> [B, C]. Wait.
        # Queue: [B, C] -> pop B -> [C]. Wait.
        # Queue: [C] -> pop C -> []. No wait.
        self.assertEqual(mock_sleep.call_count, 2)

    @patch('time.sleep')
    def test_visited_cycle_prevention(self, mock_sleep):
        # A <-> B
        self.scraper.processor.fetch_article.return_value = "html"
        self.scraper.processor.get_links.side_effect = lambda h: [
            "B"] if self.scraper.processor.fetch_article.call_count == 1 else ["A"]

        # Override side effect slightly differently to match specific calls if needed,
        # but simplier: fetch called with A returns links=[B]. fetch called with B returns links=[A].

        def links_side_effect(html):
            # This is tricky without state in side_effect, relying on call order
            # Let's assume fetch returns distinct HTML
            if html == "HTML_A": return ["B"]
            if html == "HTML_B": return ["A"]
            return []

        self.scraper.processor.fetch_article.side_effect = ["HTML_A", "HTML_B"]
        self.scraper.processor.get_links.side_effect = links_side_effect

        self.scraper.handle_auto_count("A", depth=5, wait=0)

        # Should process A, then B.
        # B links to A. A is already visited. Queue empty.
        self.assertEqual(self.scraper.processor.fetch_article.call_count, 2)


if __name__ == '__main__':
    unittest.main()
