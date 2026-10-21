import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const groups=JSON.parse(fs.readFileSync(path.join(root,'src/localization/catalog.json'),'utf8'));
const feature=groups.find(c=>c.name==='Feature');
for(const group of groups){const keys=new Set();for(const row of group.entries){if(keys.has(row[0]))throw Error('Duplicate catalog key '+row[0]);keys.add(row[0]);if(row.length!==group.fields.length||row.some(v=>typeof v!=='string'||!v.trim()))throw Error('Invalid row '+row[0]);}}
const hkSource=fs.readFileSync(path.join(root,'include/zh_hk_strings.h'),'utf8');
const hk=new Map();for(const m of hkSource.matchAll(/\{L"((?:\\.|[^"])*)",\s*L"((?:\\.|[^"])*)"\}/g)){if(hk.has(m[1])||!m[2].trim())throw Error('Invalid Hong Kong key '+m[1]);hk.set(m[1],m[2]);}
const required=new Set();
function visit(dir){for(const entry of fs.readdirSync(dir,{withFileTypes:true})){const full=path.join(dir,entry.name);if(entry.isDirectory())visit(full);else if(/\.(cpp|h)$/.test(entry.name)&&entry.name!=='catalog_data.h'){const text=fs.readFileSync(full,'utf8');for(const m of text.matchAll(/Tr\(L"((?:\\.|[^"])*)"/g))required.add(m[1]);}}}
visit(path.join(root,'src'));
for(const key of required)if(!hk.has(key))throw Error('Missing Hong Kong key '+key);
const additions=['Leave','Pending','Batch edit selected','Make-up attendance','Rename lesson','Copy lesson','Course from template','Export V5 compatible file','Imported roster entries start as Pending until marked.','V5 changes Leave and Pending to Other with notes. Older versions calculate rates differently. Export a separate copy?','Choose a separate file; the active workbook and V6 files cannot be overwritten by V5 export.','A note is required for Other and Leave.','records in current scope','Mark current scope Present?','Current scope','Backup storage is full. Export or clean ordinary backups; the existing recovery file is preserved.','Keep status','No changes: invalid input, duplicate target or data limit.','Apply changes?','Select records first.','Notes','Keep notes','Replace notes','Append notes','Reason (required):','No data','Attendance trend','Scope: Present','Choose item','Students remain unreviewed or Pending. Keep partial results?','Keys 1-6: Present, Absent, Late, Other, Leave, Pending; arrows: Previous / Skip'];
const settingsKeys=["Theme","Accent tone","Text size","Interface density","Autosave interval","Show statistics cards","Show bottom usage hint","Show table grid lines","Pure black","Pure white","Neutral","Muted sage","Muted cyan","Small","Standard","Large","Compact","Comfortable","Spacious","Off","30 seconds","1 minute","5 minutes"];
for(const key of [...additions,...settingsKeys]){const row=feature.entries.find(e=>e[0]===key);if(!row||row.length!==20||row.slice(1).some(v=>!v.trim()))throw Error('Incomplete new feature '+key);}
console.log('Localization: '+additions.length+' feature keys + '+settingsKeys.length+' settings keys × 20 languages; '+required.size+' Hong Kong call-site keys covered; no empty/duplicate rows.');
