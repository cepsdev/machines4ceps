const all_builds = require('./all_builds.json');

console.log(all_builds.allBuilds.length);
console.log(all_builds.allBuilds[0].actions[0].parameters);
console.log(all_builds.allBuilds[0]);
