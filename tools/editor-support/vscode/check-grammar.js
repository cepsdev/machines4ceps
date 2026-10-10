#!/usr/bin/env node
/*
 * Tokenize ceps files with the generated grammar and assert that the scopes
 * which matter actually land. Run after regenerating:
 *
 *     npm install vscode-textmate vscode-oniguruma
 *     node tools/editor-support/vscode/check-grammar.js
 *
 * With -v it prints every token and its scopes, which is the quickest way to
 * see why something is not coloured.
 */
const fs = require('fs');
const path = require('path');

let oniguruma, vsctm;
try {
  oniguruma = require('vscode-oniguruma');
  vsctm = require('vscode-textmate');
} catch (e) {
  console.error('check-grammar: needs vscode-textmate and vscode-oniguruma:');
  console.error('  npm install vscode-textmate vscode-oniguruma');
  process.exit(2);
}

const HERE = __dirname;
const GRAMMAR = path.join(HERE, 'syntaxes', 'ceps.tmLanguage.json');
const verbose = process.argv.includes('-v');
const files = process.argv.slice(2).filter(a => a !== '-v');
if (files.length === 0) files.push(path.join(HERE, '..', 'sample.ceps'));

/* Each case: a token that must appear somewhere, and the scope it must carry. */
const expectations = [
  ['m', 'support.constant.si-unit.ceps'],
  ['s', 'support.constant.si-unit.ceps'],
  ['kg', 'support.constant.si-unit.ceps'],
  ['mol', 'support.constant.si-unit.ceps'],
  ['cd', 'support.constant.si-unit.ceps'],
  ['A', 'entity.name.variable.ceps'],
  ['K', 'entity.name.variable.ceps'],
  ['g', 'entity.name.variable.ceps'],
  ['kind', 'keyword.declaration.kind.ceps'],
  ['Event', 'entity.name.type.ceps'],
  ['val', 'keyword.declaration.ceps'],
  ['static_for', 'keyword.control.ceps'],
  ['ldi32', 'support.function.opcode.ceps'],
  ['addi32', 'support.function.opcode.ceps'],
  ['OblectamentaDataLabel', 'storage.type.ceps'],
  ['sm', 'support.class.sm.ceps'],
  ['t', 'support.class.sm.ceps'],
  ['100', 'constant.numeric.integer.ceps'],
  ['3.14', 'constant.numeric.float.ceps'],
];

async function main() {
  const wasm = fs.readFileSync(
    path.join(path.dirname(require.resolve('vscode-oniguruma')), 'onig.wasm'));
  await oniguruma.loadWASM(wasm.buffer);

  const registry = new vsctm.Registry({
    onigLib: Promise.resolve({
      createOnigScanner: s => new oniguruma.OnigScanner(s),
      createOnigString: s => new oniguruma.OnigString(s),
    }),
    loadGrammar: async () =>
      vsctm.parseRawGrammar(fs.readFileSync(GRAMMAR, 'utf8'), GRAMMAR),
  });

  const grammar = await registry.loadGrammar('source.ceps');
  if (!grammar) {
    console.error('check-grammar: grammar failed to load');
    process.exit(1);
  }

  const seen = new Map();
  let failures = 0;

  for (const file of files) {
    const lines = fs.readFileSync(file, 'utf8').split('\n');
    let stack = vsctm.INITIAL;
    let total = 0, unscoped = 0;

    for (let i = 0; i < lines.length; i++) {
      const res = grammar.tokenizeLine(lines[i], stack);
      for (const tok of res.tokens) {
        const text = lines[i].substring(tok.startIndex, tok.endIndex);
        if (!text.trim()) continue;
        total++;
        const scopes = tok.scopes.filter(sc => sc !== 'source.ceps');
        if (scopes.length === 0) unscoped++;
        if (!seen.has(text)) seen.set(text, new Set());
        scopes.forEach(sc => seen.get(text).add(sc));
        if (verbose) {
          console.log(
            `${String(i + 1).padStart(4)}  ${JSON.stringify(text).padEnd(26)}` +
            `${scopes.join(' ') || '-'}`);
        }
      }
      stack = res.ruleStack;
    }
    console.log(`${path.basename(file)}: ${total} tokens, ${unscoped} unscoped`);
  }

  for (const [text, scope] of expectations) {
    const got = seen.get(text);
    if (!got || !got.has(scope)) {
      console.error(
        `FAIL ${JSON.stringify(text)} expected ${scope}, got ` +
        (got ? [...got].join(' ') : '(token never seen)'));
      failures++;
    }
  }

  if (failures) {
    console.error(`\ncheck-grammar: ${failures} expectation(s) failed`);
    process.exit(1);
  }
  console.log(`check-grammar: ${expectations.length} expectations hold`);
}

main().catch(e => { console.error(e); process.exit(1); });
