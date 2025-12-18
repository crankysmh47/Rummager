import { useState, useEffect, useRef } from 'react'

const LOCAL_API = "http://localhost:8000"
const CLOUD_API = "https://aether-engine.up.railway.app" // Placeholder

function App() {
  const [apiBase, setApiBase] = useState(LOCAL_API)
  const [health, setHealth] = useState("checking")
  const [query, setQuery] = useState("")
  const [results, setResults] = useState([])
  const [suggestions, setSuggestions] = useState([])
  const [searching, setSearching] = useState(false)
  const [ingesting, setIngesting] = useState(false)
  const [timeTaken, setTimeTaken] = useState(0)

  // --- HEALTH CHECK ---
  useEffect(() => {
    const checkHealth = async () => {
      try {
        const res = await fetch(LOCAL_API + "/health")
        if (res.ok) {
          setApiBase(LOCAL_API)
          setHealth("online")
        } else {
          throw new Error("Local offline")
        }
      } catch (e) {
        setApiBase(CLOUD_API)
        setHealth("cloud")
      }
    }
    checkHealth()
  }, [])

  // --- AUTOCOMPLETE ---
  useEffect(() => {
    if (query.length < 2) {
      setSuggestions([])
      return
    }
    const timer = setTimeout(async () => {
      try {
        // Extract last word for verify
        const lastWord = query.split(" ").pop()
        const res = await fetch(`${apiBase}/suggest?q=${lastWord}`)
        const data = await res.json()
        if (data.suggestions) setSuggestions(data.suggestions)
      } catch (e) { console.error(e) }
    }, 200)
    return () => clearTimeout(timer)
  }, [query, apiBase])

  // --- SEARCH ---
  const handleSearch = async (e) => {
    e.preventDefault()
    if (!query) return
    setSearching(true)
    setSuggestions([])
    try {
      const res = await fetch(`${apiBase}/search?q=${query}`)
      const data = await res.json()
      // data: { time_ms: 123, results: [...] }
      if (data.results) {
        setResults(data.results)
        setTimeTaken(data.time_ms)
      }
    } catch (e) {
      console.error("Search failed", e)
    }
    setSearching(false)
  }

  // --- UPLOAD SIMULATION ---
  const handleUpload = () => {
    setIngesting(true)
    // Simulate ingest time (async non-blocking)
    setTimeout(() => {
      setIngesting(false)
      alert("Aether Updated. New Knowledge Assimilated.")
    }, 5000)
  }

  return (
    <div className="min-h-screen bg-aether-black text-gray-200 overflow-x-hidden selection:bg-aether-red selection:text-white">

      {/* --- PULSING INGEST LINE --- */}
      {ingesting && (
        <div className="fixed top-0 left-0 w-full h-1 bg-gradient-to-r from-transparent via-aether-red to-transparent animate-pulse z-50"></div>
      )}

      {/* --- HEADER --- */}
      <div className="absolute top-4 right-4 flex items-center gap-4">
        {/* Environment Indicator */}
        <div className="flex items-center gap-2 text-xs font-mono">
          <div className={`w-2 h-2 rounded-full ${health === 'online' ? 'bg-green-500 shadow-[0_0_8px_#22c55e]' : 'bg-yellow-500'}`}></div>
          {health === 'online' ? 'CORE: LOCAL' : 'CORE: CLOUD'}
        </div>

        {/* Upload Button */}
        <button
          onClick={handleUpload}
          disabled={ingesting}
          className="px-4 py-1.5 border border-aether-dark bg-white/5 hover:bg-aether-red/20 hover:border-aether-red/50 rounded text-sm transition-all"
        >
          {ingesting ? 'ASSIMILATING...' : 'UPLOAD DATA'}
        </button>
      </div>

      <div className="max-w-4xl mx-auto px-4 pt-32 pb-12">

        {/* --- HERO TITLE --- */}
        <h1 className="text-5xl font-bold tracking-tighter text-center mb-8 text-transparent bg-clip-text bg-gradient-to-b from-white to-gray-600">
          AETHER
        </h1>

        {/* --- SEARCH BAR --- */}
        <div className="relative z-20 group">
          <div className="absolute -inset-1 bg-gradient-to-r from-aether-red to-violet-600 rounded-lg blur opacity-25 group-hover:opacity-50 transition duration-1000"></div>
          <form onSubmit={handleSearch} className="relative">
            <input
              type="text"
              value={query}
              onChange={(e) => setQuery(e.target.value)}
              placeholder="Query the database..."
              className="w-full bg-black/80 backdrop-blur-xl border border-white/10 text-white px-6 py-4 rounded-lg outline-none focus:ring-2 focus:ring-aether-red/50 focus:border-aether-red transition-all shadow-2xl text-lg placeholder:text-gray-600"
            />
            {searching && (
              <div className="absolute right-4 top-4 animate-spin h-6 w-6 border-2 border-aether-red border-t-transparent rounded-full"></div>
            )}
          </form>

          {/* SUGGESTIONS DROPDOWN */}
          {suggestions.length > 0 && (
            <div className="absolute top-full left-0 w-full mt-2 bg-black/90 border border-white/10 rounded-lg shadow-2xl backdrop-blur-md overflow-hidden animate-in fade-in zoom-in-95 duration-200">
              {suggestions.map((s, i) => (
                <div
                  key={i}
                  onClick={() => { setQuery(s); setSuggestions([]); }}
                  className="px-6 py-3 hover:bg-aether-red/20 cursor-pointer text-gray-300 hover:text-white transition-colors"
                >
                  {s}
                </div>
              ))}
            </div>
          )}
        </div>

        {/* --- STATS --- */}
        {results.length > 0 && (
          <div className="mt-4 text-center text-xs text-gray-500 font-mono">
            FOUND {results.length} RESULTS IN {timeTaken}MS
          </div>
        )}

        {/* --- RESULTS FEED --- */}
        <div className="mt-12 space-y-6">
          {results.map((doc, i) => (
            <div key={i} className="group relative p-6 bg-white/5 border border-white/5 hover:border-aether-red/50 rounded-xl transition-all duration-300 hover:shadow-[0_0_30px_-5px_rgba(244,63,94,0.15)]">
              <div className="flex justify-between items-start">
                <div>
                  <h3 className="text-xl font-bold text-gray-100 group-hover:text-aether-red transition-colors mb-2">
                    {doc.title}
                  </h3>
                  <div className="text-sm text-gray-500 mb-4">{doc.authors}</div>
                </div>
                <span className="bg-aether-red/10 text-aether-red px-2 py-1 rounded text-xs font-mono border border-aether-red/20">
                  {Number(doc.score).toFixed(4)}
                </span>
              </div>

              <div className="flex items-center gap-4 text-xs text-gray-600 font-mono">
                <span>{doc.date}</span>
                <span>{doc.category}</span>
                <span className="bg-gray-800 px-1.5 py-0.5 rounded text-gray-400">ID: {doc.id}</span>
              </div>
            </div>
          ))}
        </div>

      </div>
    </div>
  )
}

export default App
