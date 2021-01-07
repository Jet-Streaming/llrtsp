#!/usr/bin/env -S npx ts-node
import * as fs from 'fs';
import { LLParse } from 'llparse';
import * as path from 'path';
import * as semver from 'semver';

import * as llrtsp from '../src/llrtsp';

const pkgFile = path.join(__dirname, '..', 'package.json');
const pkg = JSON.parse(fs.readFileSync(pkgFile).toString());

const BUILD_DIR = path.join(__dirname, '..', 'build');
const C_DIR = path.join(BUILD_DIR, 'c');
const SRC_DIR = path.join(__dirname, '..', 'src');

const C_FILE = path.join(C_DIR, 'llrtsp.c');
const HEADER_FILE = path.join(BUILD_DIR, 'llrtsp.h');

for (const dir of [ BUILD_DIR, C_DIR ]) {
    try {
        fs.mkdirSync(dir);
    } catch (e) {
        // no-op
    }
}

function build(mode: 'strict' | 'loose') {
    const llparse = new LLParse('llrtsp__internal');
    const instance = new llrtsp.RTSP(llparse, mode);

    return llparse.build(instance.build().entry, {
        c: {
            header: 'llrtsp',
        },
        debug: process.env.LLPARSE_DEBUG ? 'llrtsp__debug' : undefined,
        generateBitcode: false,
        headerGuard: 'INCLUDE_LLRTSP_ITSELF_H_',
    });
}

function guard(strict: string, loose: string): string {
    let out = '';

    if (strict === loose) {
        return strict;
    }

    out += '#if LLRTSP_STRICT_MODE\n';
    out += '\n';
    out += strict + '\n';
    out += '\n';
    out += '#else  /* !LLRTSP_STRICT_MODE */\n';
    out += '\n';
    out += loose + '\n';
    out += '\n';
    out += '#endif  /* LLRTSP_STRICT_MODE */\n';

    return out;
}

const artifacts = {
    loose: build('loose'),
    // loose: build('strict'),
    strict: build('strict'),
};

let headers = '';

headers += '#ifndef INCLUDE_LLRTSP_H_\n';
headers += '#define INCLUDE_LLRTSP_H_\n';

headers += '\n';

const version = semver.parse(pkg.version)!;

headers += `#define LLRTSP_VERSION_MAJOR ${version.major}\n`;
headers += `#define LLRTSP_VERSION_MINOR ${version.minor}\n`;
headers += `#define LLRTSP_VERSION_PATCH ${version.patch}\n`;
headers += '\n';

headers += '#ifndef LLRTSP_STRICT_MODE\n';
headers += '# define LLRTSP_STRICT_MODE 0\n';
headers += '#endif\n';
headers += '\n';

const cHeaders = new llrtsp.CHeaders();

headers += guard(artifacts.strict.header, artifacts.loose.header);

headers += '\n';

headers += cHeaders.build();

headers += '\n';

headers += fs.readFileSync(path.join(SRC_DIR, 'native', 'api.h'));

headers += '\n';
headers += '#endif  /* INCLUDE_LLRTSP_H_ */\n';

fs.writeFileSync(C_FILE,
    guard(artifacts.strict.c || '', artifacts.loose.c || ''));
fs.writeFileSync(HEADER_FILE, headers);
