{OnigScanner} = require '../lib/oniguruma'

describe "OnigScanner", ->
  describe "::findNextMatchSync", ->
    it "returns the index of the matching pattern", ->
      scanner = new OnigScanner(["a", "b", "c"])
      expect(scanner.findNextMatchSync("x", 0)).toBe null
      expect(scanner.findNextMatchSync("xxaxxbxxc", 0).index).toBe 0
      expect(scanner.findNextMatchSync("xxaxxbxxc", 4).index).toBe 1
      expect(scanner.findNextMatchSync("xxaxxbxxc", 7).index).toBe 2
      expect(scanner.findNextMatchSync("xxaxxbxxc", 9)).toBe null

    it "includes the scanner with the results", ->
      scanner = new OnigScanner(["a"])
      expect(scanner.findNextMatchSync("a", 0).scanner).toBe scanner

    describe "when the string searched contains unicode characters", ->
      it "returns the correct matching pattern", ->
        scanner = new OnigScanner(["1", "2"])
        match = scanner.findNextMatchSync('ab…cde21', 5)
        expect(match.index).toBe 1

    it "returns false when the input string isn't a string", ->
      scanner = new OnigScanner(["1"])
      expect(scanner.findNextMatchSync()).toBe null
      expect(scanner.findNextMatchSync(null)).toBe null
      expect(scanner.findNextMatchSync(2)).toBe null
      expect(scanner.findNextMatchSync(false)).toBe null

    it "uses 0 as the start position when the input start position isn't a valid number", ->
      scanner = new OnigScanner(["1"])
      expect(scanner.findNextMatchSync('a1', Infinity).index).toBe 0
      expect(scanner.findNextMatchSync('a1', -1).index).toBe 0
      expect(scanner.findNextMatchSync('a1', false).index).toBe 0
      expect(scanner.findNextMatchSync('a1', 'food').index).toBe 0

  describe "::findNextMatch", ->
    matchCallback = null

    beforeEach ->
      matchCallback = jasmine.createSpy('matchCallback')

    it "returns the index of the matching pattern", ->
      scanner = new OnigScanner(["a", "b", "c"])
      scanner.findNextMatch("x", 0, matchCallback)

      waitsFor ->
        matchCallback.callCount is 1

      runs ->
        expect(matchCallback.argsForCall[0][0]).toBeNull()
        expect(matchCallback.argsForCall[0][1]).toBeNull()
        scanner.findNextMatch("xxaxxbxxc", 0, matchCallback)

      waitsFor ->
        matchCallback.callCount is 2

      runs ->
        expect(matchCallback.argsForCall[1][0]).toBeNull()
        expect(matchCallback.argsForCall[1][1].index).toBe 0
        scanner.findNextMatch("xxaxxbxxc", 4, matchCallback)

      waitsFor ->
        matchCallback.callCount is 3

      runs ->
        expect(matchCallback.argsForCall[2][0]).toBeNull()
        expect(matchCallback.argsForCall[2][1].index).toBe 1
        scanner.findNextMatch("xxaxxbxxc", 7, matchCallback)

      waitsFor ->
        matchCallback.callCount is 4

      runs ->
        expect(matchCallback.argsForCall[3][0]).toBeNull()
        expect(matchCallback.argsForCall[3][1].index).toBe 2
        scanner.findNextMatch("xxaxxbxxc", 9, matchCallback)

      waitsFor ->
        matchCallback.callCount is 5

      runs ->
        expect(matchCallback.argsForCall[4][0]).toBeNull()
        expect(matchCallback.argsForCall[4][1]).toBeNull()

    it "includes the scanner with the results", ->
      scanner = new OnigScanner(["a"])
      scanner.findNextMatch("a", 0, matchCallback)

      waitsFor ->
        matchCallback.callCount is 1

      runs ->
        expect(matchCallback.argsForCall[0][0]).toBeNull()
        expect(matchCallback.argsForCall[0][1].scanner).toBe scanner

    describe "when the string searched contains unicode characters", ->
      it "returns the correct matching pattern", ->
        scanner = new OnigScanner(["1", "2"])
        scanner.findNextMatch('ab…cde21', 5, matchCallback)

        waitsFor ->
          matchCallback.callCount is 1

        runs ->
          expect(matchCallback.argsForCall[0][0]).toBeNull()
          expect(matchCallback.argsForCall[0][1].index).toBe 1

    describe "::enableTracing", ->
      it "makes findNextMatchSync attach tracing results to its result", ->
        scanner = new OnigScanner(["1", "2"])
        scanner.enableTracing(0)
        slowSearches = scanner.findNextMatchSync("xxxx123", 0).slowSearches
        expect(slowSearches.length).toBe 2
        expect(slowSearches[0].index).toBe 0
        expect(slowSearches[0].pattern).toBe "1"
        expect(slowSearches[0].duration).toEqual jasmine.any(Number)
        expect(slowSearches[1].index).toBe 1
        expect(slowSearches[1].pattern).toBe "2"
        expect(slowSearches[1].duration).toEqual jasmine.any(Number)

      it "doesn't have any effect unless enabled", ->
        scanner = new OnigScanner(["1", "2"])
        expect(scanner.findNextMatchSync("xxxx123", 0).slowSearches).toBeUndefined

      describe "when it is enabled", ->
        it "has non-empty slowSearches only if it is actually slow", ->
          longEnoughThreshold = 10
          scanner = new OnigScanner(["1", "2"])
          scanner.enableTracing(longEnoughThreshold)
          expect(scanner.findNextMatchSync("xxxx123", 0).slowSearches.length).toBe 0

        it "has provides matched indices if any", ->
          scanner = new OnigScanner(["1", "y"])
          scanner.enableTracing(0)
          slowSearches = scanner.findNextMatchSync("xxxx123", 0).slowSearches
          expect(slowSearches[0].matchedAt).toBe 4
          expect(slowSearches[1].matchedAt).toBe null
