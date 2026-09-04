'use client';

/* oxlint-disable jsx-a11y/prefer-tag-over-role -- SVG treemap nodes need SVG-native interactive roles. */

import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import {
  NativeSelect,
  NativeSelectOption,
} from '@/components/ui/native-select';
import {
  hierarchy,
  treemap,
  treemapSquarify,
  type HierarchyRectangularNode,
} from 'd3-hierarchy';
import {
  Binary,
  Boxes,
  CheckCircle2,
  ClipboardList,
  Crosshair,
  Database,
  ExternalLink,
  FolderOpen,
  RotateCcw,
  Search,
  ShieldAlert,
  X,
} from 'lucide-react';
import { useEffect, useMemo, useRef, useState } from 'react';

type FunctionStatus =
  | 'candidate'
  | 'discovered'
  | 'documented'
  | 'decompiled'
  | 'matching';

/* Haritada gosterilen durum. C'den byte-matching olan fonksiyonlar
   assembly transkripsiyonundan ayrilir: ikisi de ROM'u uretir ama yalnizca
   ilki okunabilir kaynak uretir. */
type DisplayStatus = FunctionStatus | 'cMatching';

function displayStatus(fn: FunctionRecord): DisplayStatus {
  return fn.status === 'matching' && fn.cMatching ? 'cMatching' : fn.status;
}

type SourceType = 'c' | 'asm' | 'none';

const SOURCE_LABELS: Record<SourceType, string> = {
  c: 'C kaynağı',
  asm: 'Assembly',
  none: 'Kaynak yok',
};

export type FunctionRecord = {
  address: string;
  name: string;
  size: number;
  status: FunctionStatus;
  module: string;
  notes: string;
  matchedBytes: number;
  matchPercent: number;
  sourceType: SourceType;
  sourcePath: string;
  cMatching: boolean;
  cluster: string;
  clusterLabel: string;
  analysisPath?: string;
  analysisCode?: string;
  sourceCode?: string;
};

export type DashboardData = {
  summary: {
    functionCount: number;
    reviewedCount: number;
    matchingCount: number;
    totalCodeBytes: number;
    matchingCodeBytes: number;
    matchingCodePercent: number;
    matchingRegionBytes: number;
    libcRegionBytes: number;
    verifiedRomBytes: number;
    clusterCount: number;
    cSourceCount: number;
    cMatchingCount: number;
    cSourceBytes: number;
    boundaryDebtCount: number;
  };
  functions: FunctionRecord[];
  regions: Array<{ start: string; end: string; size: number; label: string }>;
  workQueue: WorkItem[];
};

type WorkItem = {
  id: string;
  priority: 'P0' | 'P1' | 'P2' | 'P3';
  area: string;
  status: 'todo' | 'in_progress' | 'blocked' | 'done';
  title: string;
  acceptance: string;
  evidence: string;
  updated: string;
};

type TreeDatum = {
  name: string;
  value?: number;
  fn?: FunctionRecord;
  children?: TreeDatum[];
};

const STATUS_META: Record<
  DisplayStatus,
  { label: string; color: string; glow: string }
> = {
  // Alti durum ayri TON almali: yesil -> turkuaz -> mavi -> kehribar ->
  // mor -> gri. Daha once iki yesil, sonra iki mavi cakisti; ayrimin
  // parlaklikta degil TONDA olmasi gerekiyor.
  cMatching: { label: "C'den eşleşiyor", color: '#6cff9e', glow: '#1aa757' },
  matching: { label: 'Assembly eşleşiyor', color: '#2ee6c8', glow: '#0d7d6b' },
  decompiled: { label: 'Yazıldı, eşleşmedi', color: '#4aa8ff', glow: '#1560a8' },
  documented: { label: 'Belgeli', color: '#f0ae3c', glow: '#7f4d0d' },
  discovered: { label: 'Keşfedildi', color: '#a777ff', glow: '#4c288e' },
  candidate: { label: 'Dokunulmadı', color: '#243441', glow: '#161f28' },
};

const MODULE_LABELS: Record<string, string> = {
  bootstrap: 'Başlangıç',
  interrupt: 'Kesme sistemi',
  save: 'Kayıt sistemi',
  sdk: 'GBA SDK',
  serialization: 'Serileştirme',
  ui: 'Arayüz',
  libc: 'C kitaplığı',
  unknown: 'Sınıflandırılmamış',
};

const GROUP_LABEL_MIN_WIDTH = 108;
const GROUP_LABEL_MIN_HEIGHT = 52;

type Grouping = 'cluster' | 'module' | 'bank';

function groupKey(fn: FunctionRecord, grouping: Grouping) {
  if (grouping === 'module') return fn.module;
  if (grouping === 'bank') return bankFor(fn.address);
  return fn.cluster;
}

function groupLabel(key: string, grouping: Grouping, functions: FunctionRecord[]) {
  if (grouping === 'module') return MODULE_LABELS[key] ?? key;
  if (grouping === 'bank') return key;
  return functions.find((fn) => fn.cluster === key)?.clusterLabel ?? key;
}

// Grup etiketi 10px mono: karakter başına ölçülen genişlik 6.6px.
// Sondaki " ↗" iki karakterlik yer kapladığı için bütçeden düşülür.
const LABEL_CHAR_WIDTH = 6.6;

function truncateToWidth(text: string, pixels: number) {
  const maxChars = Math.max(3, Math.floor(pixels / LABEL_CHAR_WIDTH) - 2);
  return text.length > maxChars ? `${text.slice(0, maxChars - 1)}…` : text;
}

function formatBytes(value: number) {
  if (value < 1024) return `${value} B`;
  return `${(value / 1024).toFixed(value < 10240 ? 2 : 1)} kB`;
}

function useElementWidth<T extends HTMLElement>() {
  const ref = useRef<T>(null);
  const [width, setWidth] = useState(900);

  useEffect(() => {
    if (!ref.current) return;
    const observer = new ResizeObserver(([entry]) => {
      setWidth(Math.max(320, Math.floor(entry.contentRect.width)));
    });
    observer.observe(ref.current);
    return () => observer.disconnect();
  }, []);

  return { ref, width };
}

function bankFor(address: string) {
  const numeric = Number.parseInt(address, 16);
  const start = Math.floor((numeric - 0x08000000) / 0x10000) * 0x10000 + 0x08000000;
  return `ROM ${start.toString(16).toUpperCase().padStart(8, '0')}`;
}

export default function DecompDashboard({ data }: { data: DashboardData }) {
  const [query, setQuery] = useState('');
  const [moduleFilter, setModuleFilter] = useState('all');
  const [statusFilter, setStatusFilter] = useState('all');
  const [sourceFilter, setSourceFilter] = useState('all');
  const [grouping, setGrouping] = useState<Grouping>('cluster');
  const [focusGroup, setFocusGroup] = useState<string | null>(null);
  const [inspectorOpen, setInspectorOpen] = useState(false);
  const [selected, setSelected] = useState<FunctionRecord>(
    data.functions.find((fn) => fn.name === 'GameInit') ?? data.functions[0],
  );
  const [tooltip, setTooltip] = useState<{
    fn: FunctionRecord;
    x: number;
    y: number;
  } | null>(null);
  const { ref: mapRef, width } = useElementWidth<HTMLDivElement>();
  const height = width < 700 ? 560 : 690;

  const modules = useMemo(
    () => [...new Set(data.functions.map((fn) => fn.module))].sort(),
    [data.functions],
  );

  const filtered = useMemo(() => {
    const normalized = query.trim().toLowerCase();
    return data.functions.filter((fn) => {
      const matchesQuery =
        !normalized ||
        fn.name.toLowerCase().includes(normalized) ||
        fn.address.toLowerCase().includes(normalized) ||
        fn.notes.toLowerCase().includes(normalized);
      return (
        matchesQuery &&
        (moduleFilter === 'all' || fn.module === moduleFilter) &&
        (statusFilter === 'all' || displayStatus(fn) === statusFilter) &&
        (sourceFilter === 'all' || fn.sourceType === sourceFilter)
      );
    });
  }, [data.functions, moduleFilter, query, sourceFilter, statusFilter]);

  const visibleFunctions = useMemo(
    () =>
      focusGroup
        ? filtered.filter((fn) => groupKey(fn, grouping) === focusGroup)
        : filtered,
    [filtered, focusGroup, grouping],
  );

  const map = useMemo(() => {
    const groups = new Map<string, FunctionRecord[]>();
    for (const fn of visibleFunctions) {
      const key = groupKey(fn, grouping);
      groups.set(key, [...(groups.get(key) ?? []), fn]);
    }
    const datum: TreeDatum = {
      name: 'ROM',
      children: [...groups.entries()].map(([name, functions]) => ({
        name,
        children: functions.map((fn) => ({ name: fn.name, value: fn.size, fn })),
      })),
    };
    const root = hierarchy<TreeDatum>(datum)
      .sum((node) => node.value ?? 0)
      .sort((a, b) => (b.value ?? 0) - (a.value ?? 0));
    return treemap<TreeDatum>()
      .size([width, height])
      .tile(treemapSquarify.ratio(1.12))
      .paddingOuter(5)
      .paddingInner(1.5)
      .paddingTop((node) =>
        node.depth === 1 &&
        node.x1 - node.x0 >= GROUP_LABEL_MIN_WIDTH &&
        node.y1 - node.y0 >= GROUP_LABEL_MIN_HEIGHT
          ? 24
          : 0,
      )
      .round(true)(root);
  }, [grouping, height, visibleFunctions, width]);

  const resetFilters = () => {
    setQuery('');
    setModuleFilter('all');
    setStatusFilter('all');
    setSourceFilter('all');
    setFocusGroup(null);
  };

  const activeTask = data.workQueue.find((task) => task.status === 'in_progress');
  const nextTasks = data.workQueue.filter((task) => task.status === 'todo').slice(0, 4);

  return (
    <main className="dashboard-shell">
      <header className="topbar">
        <div className="brand-lockup">
          <div className="brand-mark" aria-hidden="true">GTA</div>
          <div>
            <p>Grand Theft Auto Advance</p>
            <h1>Decomp Map</h1>
          </div>
        </div>
        <div className="live-state"><span /> {formatBytes(data.summary.verifiedRomBytes)} kaynak doğrulandı</div>
      </header>

      <section className="summary-grid" aria-label="Proje özeti">
        <article className="stat-card">
          <Boxes aria-hidden="true" />
          <div><strong>{data.summary.functionCount.toLocaleString('tr-TR')}</strong><span>Fonksiyon haritası</span></div>
        </article>
        <article className="stat-card">
          <CheckCircle2 aria-hidden="true" />
          <div><strong>{data.summary.matchingCount}</strong><span>Byte-eşleşen fonksiyon</span></div>
        </article>
        <article className="stat-card">
          <FolderOpen aria-hidden="true" />
          <div><strong>{data.summary.cSourceCount}</strong><span>C kaynağı · {data.summary.cMatchingCount} eşleşen</span></div>
        </article>
        <article className="stat-card stat-card-accent">
          <Crosshair aria-hidden="true" />
          <div><strong>%{data.summary.matchingCodePercent.toFixed(2)}</strong><span>Fonksiyon gövdesi eşleşmesi</span></div>
        </article>
        <article className="stat-card">
          <Database aria-hidden="true" />
          <div><strong>{formatBytes(data.summary.verifiedRomBytes)}</strong><span>Doğrulanmış ROM bölgesi</span></div>
        </article>
        <article className="stat-card stat-card-warning">
          <ShieldAlert aria-hidden="true" />
          <div><strong>{data.summary.boundaryDebtCount}</strong><span>Açık sınır bulgusu</span></div>
        </article>
      </section>

      <section className="queue-panel" aria-label="Aktif çalışma kuyruğu">
        <div className="queue-heading">
          <div><ClipboardList aria-hidden="true" /><span>Tek aktif iş</span></div>
          <strong>{activeTask?.id ?? 'Aktif iş yok'}</strong>
        </div>
        {activeTask ? (
          <div className="active-task">
            <div><Badge variant="outline">{activeTask.priority}</Badge><h2>{activeTask.title}</h2></div>
            <p>Bitti sayılması için: {activeTask.acceptance}</p>
          </div>
        ) : <p className="queue-empty">Yeni işe başlamadan önce kuyruktan tek bir kayıt aktif yapılmalı.</p>}
        {nextTasks.length > 0 && (
          <div className="next-tasks">
            <span>Sıradaki işler</span>
            <ol>{nextTasks.map((task) => <li key={task.id}><b>{task.id}</b><span>{task.title}</span><small>{task.priority}</small></li>)}</ol>
          </div>
        )}
      </section>

      <section className="control-panel" aria-label="Harita filtreleri">
        <div className="search-wrap">
          <Search aria-hidden="true" />
          <Input
            aria-label="Fonksiyon ara"
            value={query}
            onChange={(event) => setQuery(event.target.value)}
            placeholder="Fonksiyon, adres veya not ara…"
          />
        </div>
        <NativeSelect value={moduleFilter} onChange={(event) => setModuleFilter(event.target.value)} aria-label="Modül filtresi">
          <NativeSelectOption value="all">Tüm modüller</NativeSelectOption>
          {modules.map((module) => <NativeSelectOption value={module} key={module}>{MODULE_LABELS[module] ?? module}</NativeSelectOption>)}
        </NativeSelect>
        <NativeSelect value={statusFilter} onChange={(event) => setStatusFilter(event.target.value)} aria-label="Durum filtresi">
          <NativeSelectOption value="all">Tüm durumlar</NativeSelectOption>
          {Object.entries(STATUS_META).map(([status, meta]) => <NativeSelectOption value={status} key={status}>{meta.label}</NativeSelectOption>)}
        </NativeSelect>
        <NativeSelect value={sourceFilter} onChange={(event) => setSourceFilter(event.target.value)} aria-label="Kaynak türü filtresi">
          <NativeSelectOption value="all">Tüm kaynak türleri</NativeSelectOption>
          <NativeSelectOption value="c">C kaynağı</NativeSelectOption>
          <NativeSelectOption value="asm">Assembly</NativeSelectOption>
          <NativeSelectOption value="none">Kaynak yok</NativeSelectOption>
        </NativeSelect>
        <NativeSelect value={grouping} onChange={(event) => { setGrouping(event.target.value as Grouping); setFocusGroup(null); }} aria-label="Gruplama">
          <NativeSelectOption value="cluster">Bitişik bloğa göre grupla</NativeSelectOption>
          <NativeSelectOption value="module">Modüle göre grupla</NativeSelectOption>
          <NativeSelectOption value="bank">ROM bankına göre grupla</NativeSelectOption>
        </NativeSelect>
        <Button variant="outline" onClick={resetFilters}><RotateCcw aria-hidden="true" /> Sıfırla</Button>
      </section>

      <section className="workspace-grid">
        <article className="map-card">
          <div className="map-heading">
            <div>
              <span>ROM / ARM7TDMI</span>
              <h2>{focusGroup ? <button className="breadcrumb-button" onClick={() => setFocusGroup(null)}>Tüm harita</button> : 'Fonksiyon treemap’i'}{focusGroup && <> / {groupLabel(focusGroup, grouping, data.functions)}</>}</h2>
            </div>
            <Badge variant="outline">{visibleFunctions.length.toLocaleString('tr-TR')} / {data.summary.functionCount.toLocaleString('tr-TR')}</Badge>
          </div>

          <div className="treemap-wrap" ref={mapRef}>
            {visibleFunctions.length ? (
              <svg viewBox={`0 0 ${width} ${height}`} role="img" aria-label="Fonksiyonların byte büyüklüğüne göre alan haritası">
                <defs>
                  {Object.entries(STATUS_META).map(([status, meta]) => (
                    <radialGradient id={`fill-${status}`} key={status} cx="50%" cy="42%" r="70%">
                      <stop offset="0%" stopColor={meta.color} />
                      <stop offset="100%" stopColor={meta.glow} />
                    </radialGradient>
                  ))}
                </defs>
                {(map.children ?? []).map((group) => {
                  const node = group as HierarchyRectangularNode<TreeDatum>;
                  return <g
                    className="group-cell"
                    key={node.data.name}
                    role="button"
                    tabIndex={0}
                    aria-label={`${node.data.name} grubuna gir`}
                    onClick={() => setFocusGroup(node.data.name)}
                    onKeyDown={(event) => { if (event.key === 'Enter' || event.key === ' ') setFocusGroup(node.data.name); }}
                  >
                    <rect className="group-rect" x={node.x0} y={node.y0} width={node.x1 - node.x0} height={node.y1 - node.y0} />
                    {node.x1 - node.x0 >= GROUP_LABEL_MIN_WIDTH && node.y1 - node.y0 >= GROUP_LABEL_MIN_HEIGHT && (
                      <text className="group-label" x={node.x0 + 8} y={node.y0 + 16}>
                        {truncateToWidth(
                          groupLabel(node.data.name, grouping, visibleFunctions),
                          node.x1 - node.x0 - 18,
                        )} ↗
                      </text>
                    )}
                  </g>;
                })}
                {map.leaves().map((leaf) => {
                  const node = leaf as HierarchyRectangularNode<TreeDatum>;
                  const fn = node.data.fn!;
                  const cellWidth = node.x1 - node.x0;
                  const cellHeight = node.y1 - node.y0;
                  const active = selected.address === fn.address;
                  return (
                    <g
                      className="function-cell"
                      key={fn.address}
                      role="button"
                      tabIndex={0}
                      aria-label={`${fn.name}, ${formatBytes(fn.size)}, ${STATUS_META[displayStatus(fn)].label}`}
                      onClick={() => setSelected(fn)}
                      onDoubleClick={() => { setSelected(fn); setInspectorOpen(true); }}
                      onKeyDown={(event) => { if (event.key === 'Enter' || event.key === ' ') setSelected(fn); }}
                      onPointerMove={(event) => setTooltip({ fn, x: event.clientX, y: event.clientY })}
                      onPointerLeave={() => setTooltip(null)}
                    >
                      <rect className={active ? 'leaf-rect leaf-selected' : 'leaf-rect'} x={node.x0} y={node.y0} width={cellWidth} height={cellHeight} fill={`url(#fill-${displayStatus(fn)})`} />
                      {cellWidth > 76 && cellHeight > 38 && <>
                        <text className="leaf-label" x={node.x0 + 7} y={node.y0 + 17}>{fn.name.length > 22 ? `${fn.name.slice(0, 20)}…` : fn.name}</text>
                        {cellHeight > 57 && <text className="leaf-meta" x={node.x0 + 7} y={node.y0 + 34}>{formatBytes(fn.size)} · %{fn.matchPercent.toFixed(0)}</text>}
                      </>}
                    </g>
                  );
                })}
              </svg>
            ) : <div className="empty-map">Bu filtrelerle eşleşen fonksiyon yok.</div>}
          </div>

          <div className="legend">
            {Object.entries(STATUS_META).map(([status, meta]) => <span key={status}><i style={{ background: meta.color }} />{meta.label}</span>)}
            <small>Dikdörtgen alanı = fonksiyon byte büyüklüğü</small>
          </div>
        </article>

        <aside className="detail-card" aria-live="polite">
          <div className="detail-kicker"><Binary aria-hidden="true" /> Seçili fonksiyon</div>
          <h2>{selected.name}</h2>
          <Badge className={`status-${displayStatus(selected)}`}>{STATUS_META[displayStatus(selected)].label}</Badge>
          <div className="detail-progress"><div><span>Byte eşleşmesi</span><strong>%{selected.matchPercent.toFixed(2)}</strong></div><div className="progress-track"><i style={{ width: `${selected.matchPercent}%` }} /></div></div>
          <dl>
            <div><dt>ROM adresi</dt><dd>{selected.address}</dd></div>
            <div><dt>Boyut</dt><dd>{formatBytes(selected.size)} ({selected.size} byte)</dd></div>
            <div><dt>Kaynak türü</dt><dd>{SOURCE_LABELS[selected.sourceType]}</dd></div>
            <div><dt>Kaynak yolu</dt><dd>{selected.sourcePath || '—'}</dd></div>
            <div><dt>Modül</dt><dd>{MODULE_LABELS[selected.module] ?? selected.module}</dd></div>
            <div><dt>Durum</dt><dd>{STATUS_META[displayStatus(selected)].label}</dd></div>
          </dl>
          <div className="detail-note"><span>Analiz notu</span><p>{selected.notes || 'Henüz açıklama eklenmedi.'}</p></div>
          <Button className="inspect-button" onClick={() => setInspectorOpen(true)}><FolderOpen aria-hidden="true" /> Fonksiyonun içine gir</Button>
          <div className="project-progress">
            <div className="project-note">
              {formatBytes(data.summary.matchingRegionBytes)} kaynaktan yeniden üretiliyor,
              {' '}{formatBytes(data.summary.libcRegionBytes)} standart kütüphaneye karşı doğrulandı
            </div>
            <div><span>Toplam kaynak ilerlemesi</span><strong>%{data.summary.matchingCodePercent.toFixed(2)}</strong></div>
            <div className="project-track"><i style={{ width: `${data.summary.matchingCodePercent}%` }} /></div>
            <small>{formatBytes(data.summary.matchingCodeBytes)} / {formatBytes(data.summary.totalCodeBytes)} fonksiyon gövdesi</small>
          </div>
        </aside>
      </section>

      {tooltip && <div className="map-tooltip" style={{ left: Math.min(tooltip.x + 14, window.innerWidth - 270), top: tooltip.y + 14 }}><strong>{tooltip.fn.name}</strong><span>{tooltip.fn.address} · {formatBytes(tooltip.fn.size)} · %{tooltip.fn.matchPercent.toFixed(2)}</span></div>}
      {inspectorOpen && (
        <div className="inspector-backdrop" role="presentation" onMouseDown={(event) => { if (event.target === event.currentTarget) setInspectorOpen(false); }}>
          <section className="function-inspector" role="dialog" aria-modal="true" aria-labelledby="inspector-title">
            <header>
              <div><span>Fonksiyon görünümü</span><h2 id="inspector-title">{selected.name}</h2><p>{selected.address} · {formatBytes(selected.size)} · {MODULE_LABELS[selected.module] ?? selected.module}</p></div>
              <Button variant="ghost" size="icon" onClick={() => setInspectorOpen(false)} aria-label="Fonksiyon görünümünü kapat"><X aria-hidden="true" /></Button>
            </header>
            <div className="inspector-toolbar">
              <Badge className={`status-${displayStatus(selected)}`}>{STATUS_META[displayStatus(selected)].label}</Badge>
              <span>{selected.sourcePath || selected.analysisPath || 'Kaynak veya Ghidra çıktısı henüz yok'}</span>
            </div>
            {selected.sourceCode ? (
              <pre><code>{selected.sourceCode}</code></pre>
            ) : selected.analysisCode ? (
              <pre><code>{selected.analysisCode}</code></pre>
            ) : (
              <div className="no-analysis"><ExternalLink aria-hidden="true" /><h3>Bu fonksiyon henüz açılmadı</h3><p>GBA bir engel değil. Fonksiyon Ghidra’da analiz edilip C çıktısı dışa aktarıldığında kod burada görünecek. Şimdilik adresi ve sınırı otomatik analizden geliyor.</p></div>
            )}
          </section>
        </div>
      )}
    </main>
  );
}
