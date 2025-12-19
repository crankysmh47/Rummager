import { useState, useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { Server, Cloud, Search, ArrowRight, ArrowLeft, Filter, Calendar } from 'lucide-react'

// --- CONFIG ---
const LOCAL_API = import.meta.env.DEV ? "http://localhost:8000" : ""
const CLOUD_API = "https://aether-engine.up.railway.app"
const PAGE_SIZE = 20

function App() {
  // --- STATE ---
  const [serverMode, setServerMode] = useState("local") // auto, local, cloud
  const [apiBase, setApiBase] = useState(LOCAL_API)
  const [health, setHealth] = useState("checking")

  const [query, setQuery] = useState("")
  const [sortByDate, setSortByDate] = useState(false) // New
  const [catFilter, setCatFilter] = useState("")      // New

  const [allResults, setAllResults] = useState([])    // Stores up to 120 results
  const [page, setPage] = useState(0)                 // Current page (0-indexed)

  const [suggestions, setSuggestions] = useState([])
  const [searching, setSearching] = useState(false)
  const [timeTaken, setTimeTaken] = useState(0)

  // --- HEALTH & SERVER CHECK ---
  useEffect(() => {
    const checkServer = async () => {
      setHealth("checking")

      let targetApi = LOCAL_API
      if (serverMode === "cloud") targetApi = CLOUD_API

      if (serverMode === "auto") {
        // Try local first
        try {
          const res = await fetch(LOCAL_API + "/health")
          if (res.ok) {
            targetApi = LOCAL_API
            setHealth("online")
          } else {
            throw new Error("Local offline")
          }
        } catch (e) {
          targetApi = CLOUD_API // Fallback
          setHealth("cloud")
        }
      } else {
        // Manual Mode
        setApiBase(targetApi)
        try {
          await fetch(targetApi + "/health")
          setHealth(serverMode === "local" ? "online" : "cloud")
        } catch (e) {
          setHealth("offline")
        }
      }
      setApiBase(targetApi)
    }

    checkServer()
  }, [serverMode])

  // --- AUTOCOMPLETE ---
  useEffect(() => {
    if (query.length < 2) {
      setSuggestions([])
      return
    }
    const timer = setTimeout(async () => {
      try {
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
    e?.preventDefault()
    if (!query) return

    setSearching(true)
    setSuggestions([])
    setPage(0) // Reset page on new search

    try {
      // Construct Query with Flags
      let cleanQuery = query
      if (sortByDate) cleanQuery += " /date"
      if (catFilter) cleanQuery += ` /cat:${catFilter}`

      const res = await fetch(`${apiBase}/search?q=${encodeURIComponent(cleanQuery)}`)
      const data = await res.json()

      if (data.results) {
        setAllResults(data.results)
        setTimeTaken(data.time_ms)
      } else {
        setAllResults([])
      }
    } catch (e) {
      console.error("Search failed", e)
      setAllResults([])
    }
    setSearching(false)
  }

  // --- PAGINATION ---
  const visibleResults = allResults.slice(page * PAGE_SIZE, (page + 1) * PAGE_SIZE)
  const totalPages = Math.ceil(allResults.length / PAGE_SIZE)

  return (
    <div className="min-h-screen bg-nebula text-gray-200 overflow-x-hidden selection:bg-rose-500 selection:text-white font-sans">

      {/* --- HEADER --- */}
      <div className="fixed top-0 w-full z-50 px-6 py-4 flex justify-between items-center bg-black/20 backdrop-blur-md border-b border-white/5">
        <div className="font-bold text-xl tracking-tighter text-transparent bg-clip-text bg-gradient-to-r from-rose-500 to-violet-500">
          AETHER
        </div>

        {/* Server Switcher */}
        <div className="flex items-center gap-4 bg-black/40 rounded-full px-2 py-1 border border-white/10">
          <button
            onClick={() => setServerMode("auto")}
            className={`px-3 py-1 rounded-full text-xs transition-all ${serverMode === 'auto' ? 'bg-white/10 text-white' : 'text-gray-500 hover:text-gray-300'}`}
          >
            AUTO
          </button>
          <button
            onClick={() => setServerMode("local")}
            className={`p-1.5 rounded-full transition-all ${serverMode === 'local' ? 'bg-rose-500/20 text-rose-400' : 'text-gray-500 hover:text-gray-300'}`}
            title="Force Localhost"
          >
            <Server size={14} />
          </button>
          <button
            onClick={() => setServerMode("cloud")}
            className={`p-1.5 rounded-full transition-all ${serverMode === 'cloud' ? 'bg-blue-500/20 text-blue-400' : 'text-gray-500 hover:text-gray-300'}`}
            title="Force Cloud"
          >
            <Cloud size={14} />
          </button>

          {/* Status Dot */}
          <div className={`w-2 h-2 rounded-full ml-1 ${health.includes('online') || health === 'cloud' ? 'bg-green-500 shadow-[0_0_8px_#22c55e]' : 'bg-red-500 animate-pulse'}`}></div>
        </div>
      </div>

      <div className="max-w-5xl mx-auto px-4 pt-32 pb-12">

        {/* --- HERO SECTION --- */}
        <motion.div
          initial={{ opacity: 0, y: -20 }}
          animate={{ opacity: 1, y: 0 }}
          className="text-center mb-12"
        >
          <h1 className="text-6xl font-black tracking-tighter mb-4">
            <span className="text-white">ACCESS</span>
            <span className="text-transparent bg-clip-text bg-gradient-to-r from-rose-500 to-violet-600"> KNOWLEDGE</span>
          </h1>
          <p className="text-gray-500 font-mono text-sm max-w-md mx-auto">
            Decentralized Academic Search Engine. <br />
            Connected to Core: <span className="text-rose-400">{apiBase}</span>
          </p>
        </motion.div>

        {/* --- SEARCH BAR --- */}
        <div className="max-w-2xl mx-auto relative z-20 group mb-8">
          <div className="absolute -inset-1 bg-gradient-to-r from-rose-500 to-violet-600 rounded-xl blur opacity-25 group-hover:opacity-50 transition duration-1000"></div>
          <form onSubmit={handleSearch} className="relative flex items-center bg-black/80 backdrop-blur-xl border border-white/10 rounded-xl overflow-hidden shadow-2xl">
            <Search className="ml-4 text-gray-500" size={20} />
            <input
              type="text"
              value={query}
              onChange={(e) => setQuery(e.target.value)}
              placeholder="Search metadata, authors, or IDs..."
              className="w-full bg-transparent text-white px-4 py-4 outline-none text-lg placeholder:text-gray-600"
            />
            {searching && (
              <div className="absolute right-4 animate-spin h-5 w-5 border-2 border-rose-500 border-t-transparent rounded-full"></div>
            )}
          </form>

          {/* SUGGESTIONS */}
          <AnimatePresence>
            {suggestions.length > 0 && (
              <motion.div
                initial={{ opacity: 0, height: 0 }}
                animate={{ opacity: 1, height: 'auto' }}
                exit={{ opacity: 0, height: 0 }}
                className="absolute top-full left-0 w-full mt-2 bg-black/90 border border-white/10 rounded-lg overflow-hidden backdrop-blur-md z-30"
              >
                {suggestions.map((s, i) => (
                  <div
                    key={i}
                    onClick={() => {
                      // Multi-word support: replace only the last partial word
                      const words = query.trim().split(/\s+/);
                      words.pop(); // Remove partial
                      words.push(s); // Add suggestion
                      setQuery(words.join(" ") + " "); // Add space for next word
                      setSuggestions([]);
                    }}
                    className="px-6 py-3 hover:bg-rose-500/10 cursor-pointer text-gray-300 hover:text-white transition-colors border-b border-white/5 last:border-0"
                  >
                    {s}
                  </div>
                ))}
              </motion.div>
            )}
          </AnimatePresence>
        </div>

        {/* --- UPLOAD BUTTON --- */}
        <div className="flex justify-center mb-8">
          <label className="flex items-center gap-2 px-4 py-2 bg-white/5 hover:bg-white/10 border border-white/10 rounded-lg cursor-pointer transition-all text-sm text-gray-400 hover:text-white hover:border-white/20">
            <span className="text-xl font-thin">+</span>
            <span>Add Document</span>
            <input
              type="file"
              className="hidden"
              onChange={async (e) => {
                const file = e.target.files[0];
                if (!file) return;
                const formData = new FormData();
                formData.append('file', file);
                try {
                  const res = await fetch(`${apiBase}/upload`, { method: 'POST', body: formData });
                  const d = await res.json();
                  alert(d.message || "Upload started");
                } catch (err) {
                  alert("Upload failed");
                  console.error(err);
                }
              }}
            />
          </label>
        </div>

        {/* --- FILTERS --- */}
        <div className="flex justify-center gap-4 mb-12">
          <button
            onClick={() => setSortByDate(!sortByDate)}
            className={`flex items-center gap-2 px-4 py-2 rounded-lg border text-sm transition-all ${sortByDate ? 'border-rose-500 bg-rose-500/10 text-rose-400' : 'border-white/10 bg-white/5 text-gray-400 hover:bg-white/10'}`}
          >
            <Calendar size={14} />
            Sort by Date
          </button>

          <div className="relative group">
            <Filter className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-500" size={14} />
            <input
              type="text"
              placeholder="Filter Category (e.g. cs.AI)"
              value={catFilter}
              onChange={(e) => setCatFilter(e.target.value)}
              className="pl-9 pr-4 py-2 bg-white/5 border border-white/10 rounded-lg text-sm text-white focus:border-rose-500 outline-none w-64 transition-all"
            />
          </div>
        </div>

        {/* --- STATS --- */}
        {allResults.length > 0 && (
          <div className="flex justify-between items-end mb-6 px-2 border-b border-white/5 pb-2">
            <div className="text-xs text-gray-500 font-mono">
              FOUND {allResults.length} RESULTS IN {timeTaken}MS
            </div>
            <div className="text-xs text-gray-500">
              PAGE {page + 1} OF {totalPages}
            </div>
          </div>
        )}

        {/* --- RESULTS FEED --- */}
        <div className="space-y-4">
          <AnimatePresence mode='wait'>
            {visibleResults.map((doc, i) => (
              <motion.div
                key={doc.id}
                initial={{ opacity: 0, y: 20 }}
                animate={{ opacity: 1, y: 0 }}
                exit={{ opacity: 0, y: -20 }}
                transition={{ delay: i * 0.05 }}
                className="group relative p-6 glass-panel rounded-xl hover-glow transition-all duration-300"
              >
                <div className="flex justify-between items-start">
                  <div className="flex-1">
                    <a
                      href={`https://arxiv.org/abs/${doc.id}`}
                      target="_blank"
                      rel="noreferrer"
                      className="text-xl font-bold text-gray-100 group-hover:text-rose-500 transition-colors mb-2 block decoration-rose-500/50 hover:underline underline-offset-4"
                    >
                      {doc.title}
                    </a>
                    <div className="text-sm text-gray-400 mb-4 flex items-center gap-2">
                      <span className="w-1 h-1 bg-rose-500 rounded-full"></span>
                      {doc.authors}
                    </div>
                  </div>
                  <span className="bg-rose-500/10 text-rose-400 px-2 py-1 rounded text-xs font-mono border border-rose-500/20 whitespace-nowrap ml-4">
                    SCORE: {Number(doc.score).toFixed(2)}
                  </span>
                </div>

                <div className="flex items-center gap-4 text-xs text-gray-600 font-mono mt-2">
                  <span className="bg-white/5 px-2 py-1 rounded text-gray-400 border border-white/5">{doc.date}</span>
                  <span className="bg-white/5 px-2 py-1 rounded text-gray-400 border border-white/5">{doc.category}</span>
                  <span className="text-gray-700">ID: {doc.id}</span>
                </div>
              </motion.div>
            ))}
          </AnimatePresence>
        </div>

        {/* --- PAGINATION CONTROLS --- */}
        {totalPages > 1 && (
          <div className="flex justify-center items-center gap-4 mt-12">
            <button
              onClick={() => setPage(p => Math.max(0, p - 1))}
              disabled={page === 0}
              className="p-3 rounded-full bg-white/5 border border-white/10 hover:bg-rose-500/20 hover:border-rose-500/50 disabled:opacity-30 disabled:hover:bg-transparent transition-all"
            >
              <ArrowLeft size={20} />
            </button>
            <span className="font-mono text-gray-500 text-sm">
              {page + 1} / {totalPages}
            </span>
            <button
              onClick={() => setPage(p => Math.min(totalPages - 1, p + 1))}
              disabled={page === totalPages - 1}
              className="p-3 rounded-full bg-white/5 border border-white/10 hover:bg-rose-500/20 hover:border-rose-500/50 disabled:opacity-30 disabled:hover:bg-transparent transition-all"
            >
              <ArrowRight size={20} />
            </button>
          </div>
        )}

      </div>
    </div>
  )
}

export default App
