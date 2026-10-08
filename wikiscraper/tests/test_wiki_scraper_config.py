import unittest
from unittest.mock import patch, MagicMock
from src.wiki_scraper import WikiScraper, get_arg_parser


class TestWikiScraperConfig(unittest.TestCase):
    def test_custom_wiki_url_logic(self):
        # Directly test the initialization logic that happens in main
        args = MagicMock()
        args.wiki = 'https://custom.wiki/idx/'

        with patch('src.wiki_scraper.ArticleProcessor') as mock_processor:
            scraper = WikiScraper(base_url=args.wiki)
            mock_processor.assert_called_once_with(base_url='https://custom.wiki/idx/')

    def test_default_wiki_url_logic(self):
        with patch('src.wiki_scraper.ArticleProcessor') as mock_processor:
            scraper = WikiScraper(base_url=None)
            mock_processor.assert_called_once_with()  # called with defaults

    def test_parser_setup(self):
        parser = get_arg_parser()
        # Verify parser has --wiki
        actions = [a.dest for a in parser._actions]
        self.assertIn('wiki', actions)


if __name__ == '__main__':
    unittest.main()
