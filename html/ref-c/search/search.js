/* Custom Doxygen local full-text search.
 * Sensory, Inc. http://sensory.com/
 *
 * Adapted from DoxygenDeepSearch
 * https://github.com/divideconcept/DoxygenDeepSearch/blob/master/search.js
 * which appears to be an adaptation of Doxygen's
 * https://github.com/doxygen/doxygen/blob/master/templates/html/extsearch.js
 */

/* Open Source Licenses
 */

/* Doxygen
 *
 * @licstart The following is the entire license notice for the
 * JavaScript code in this file.
 *
 * Copyright (C) 1997-2017 by Dimitri van Heesch
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @licend  The above is the entire license notice
 * for the JavaScript code in this file
 */

/* DoxygenDeepSearch
 *
 * MIT License
 *
 * Copyright (c) 2016 Robin Lobel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

var fragmentContext = 40;
var fragmentCountLimit = 3;
var fragmentScoreLimit = 5;
var altSearch = ' in the C documentation section. '
altSearch += 'Search the <a href="../ref-java/search.html?query=$query">';
altSearch += 'Java documentation</a> instead.';

var typeOrder = [
  'function', 'define', 'typedef', 'enum', 'enumvalue',
  'struct', 'variable', 'group', 'page', 'source', 'file'
];

var typeNames = {
  'function': 'Functions',
  'define': 'Preprocessor macros',
  'typedef': 'Type definitions',
  'enum': 'Enumerations',
  'enumvalue': 'Enumeration values',
  'struct': 'Structs',
  'variable': 'Struct members',
  'group': 'Documenation sections',
  'page': 'Pages',
  'source': 'Source code',
  'file': 'File',
};

var resultText = [
  '<p>Found no references to "$query"' + altSearch + '</p>',
  '<p>Found <b>one</b> reference to "$query"' + altSearch + '</p>',
  '<p>Found <b>$num</b> references to "$query"' + altSearch + '</p>'
];

var abbreviationMap = {
  "index": "Overview",
  "build": "Custom Recognition",
  "changelog": "Changelog",
  "tools": "Command-line Tools",
  "concept-model": "Conceptual Model",
  "contact": "Contact Information",
  "faq": "Frequently Asked Questions",
  "known-issues": "Known Issues",
  "license": "License Agreement",
  "models": "Task Models",
  "quick-start": "Quick Start",
  "task": "Task Descriptions",

  "java": "Java API",
  "java-session": "Session",
  "java-stream": "Stream",

  "c": "C API",
  "libsettings": "Library Settings",
  "task": "Task Settings",
  "task-spot": "Phrase Spotter Task",
  "task-enroll": "Enrollment Task",
  "task-vad": "Voice Activity Detection Task",
  "task-templates": "Phrase Spotter Templates Tasks",
  "task-phrasespot-vad": "Phrase Spotter with Voice Activity Detection Task",
  "build-info": "Version Macros",
  "session": "Session",
  "callback": "User Callbacks",
  "stream": "Stream",
  "stream-ops": "Stream Operations",
  "stream-provider": "Stream Type Constructors",
  "stream-spi": "Stream Provider API",
  "config": "Library Configuration",
};

function SearchBox(name, resultsPath, inFrame, label) {
  this.searchLabel = label;
  this.DOMSearchField = function () { return document.getElementById("MSearchField"); }
  this.DOMSearchBox = function () { return document.getElementById("MSearchBox"); }
  this.OnSearchFieldFocus = function (isActive) {
    if (isActive) {
      this.DOMSearchBox().className = 'MSearchBoxActive';
      var searchField = this.DOMSearchField();
      if (searchField.value == this.searchLabel) {
        searchField.value = '';
      }
    } else {
      this.DOMSearchBox().className = 'MSearchBoxInactive';
      this.DOMSearchField().value = this.searchLabel;
    }
  }
}

function trim(s) {
  return s ? s.replace(/^\s\s*/, '').replace(/\s\s*$/, '') : '';
}

function getURLParameter(name) {
  return decodeURIComponent((new RegExp('[?|&]' + name +
    '=' + '([^&;]+?)(&|#|;|$)').exec(location.search)
    || [, ""])[1].replace(/\+/g, '%20')) || null;
}

var entityMap = {
  "&": "&amp;",
  "<": "&lt;",
  ">": "&gt;",
  '"': '&quot;',
  "'": '&#39;',
  "/": '&#x2F;'
};

function escapeHtml(s) {
  return String(s).replace(/[&<>"'\/]/g, function (s) {
    return entityMap[s];
  });
}

function searchFor(query, page, count) {
  if (trim(query).length < 3) {
    var results = $('#searchresults');
    results.html('<p>Query too short (3 chars mininum).</p>');
    return;
  }

  var xmlData = document.getElementById('searchdata').innerHTML;
  if (xmlData.indexOf("<add>") < 0) {
    console.log("Search database not found in search.html")
    return;
  }
  xmlData = xmlData.substring(xmlData.indexOf("<add>"), xmlData.indexOf("</add>") + 6);

  var xmlParser = new DOMParser().parseFromString(xmlData, "text/xml");

  var doc = xmlParser.getElementsByTagName("doc");
  var out = [];

  for (i = 0; i < doc.length; i++) {
    type = name = args = tag = url = keywords = text = "";
    for (var nodeIdx = 0;
         nodeIdx < doc[i].getElementsByTagName("field").length;
         nodeIdx++) {
      node = doc[i].getElementsByTagName("field")[nodeIdx];
      if (node.childNodes[0] == undefined) continue;
      this[node.getAttribute("name")] = node.childNodes[0].nodeValue;
    }
    var kwmatch = keywords.toLowerCase().indexOf(query.toLowerCase());
    if (text.toLowerCase().indexOf(query.toLowerCase()) >= 0 || kwmatch >= 0) {
      var start = text.toLowerCase().indexOf(query.toLowerCase());
      var fragmentcount = 0;
      var frag = [];
      while (start >= 0 && fragmentcount < fragmentScoreLimit) {
        if (fragmentcount < fragmentCountLimit) {
          quotestart = Math.max(start - fragmentContext, 0);
          quoteend = Math.min(start + query.length + fragmentContext, text.length);
          fragment = '';
          if (quotestart > 0)
            fragment += '...';
          fragment += escapeHtml(text.substring(quotestart, start));
          fragment += '<span class="hl">';
          fragment += escapeHtml(text.substring(start, start + query.length));
          fragment += '</span>';
          fragment +=
            escapeHtml(text.substring(start + query.length, quoteend));
          if (quoteend < text.length) fragment += '...';
          frag.push(fragment);
        }
        start = text.toLowerCase().indexOf(query.toLowerCase(), start + 1);
        fragmentcount++;
      }
      if (kwmatch >= 0 && fragmentcount == 0) {
        tmp = text.substring(0, 2 * fragmentContext + query.length);
        if (tmp.length < text.length) tmp += '...';
        frag.push(escapeHtml(tmp));
      }
      out.push({
        type: type,
        name: name,
        args: args,
        tag: tag,
        url: url,
        frag: frag,
        score: fragmentcount + (kwmatch >= 0 ? 10 : 0),
      });
    }
  }

  out.sort(function (a, b) {
    res = b.score - a.score;
    if (res == 0) {
      res = typeOrder.indexOf(a.type) - typeOrder.indexOf(b.type);
    }
    return res;
  });

  output = '<table class="ftsearch">';
  count = 0;
  for (var resIdx = 0; resIdx < out.length; resIdx++) {
    res = out[resIdx];
    count++;
    output += '<tr class="match">';
    output += '<td class="counter">' + count + '.</td>';
    output += '<td>';
    output += '<a href="' + escapeHtml(res.url) + '">';
    if (res.name in abbreviationMap) {
      output += escapeHtml(abbreviationMap[res.name]);
    } else {
      output += escapeHtml(res.name);
    }
    if (res.args.length && (res.type === 'function' || res.type === 'define'))
      output += escapeHtml(res.args);
    output += '</a>';
    output += '<span class="type">';
    output += escapeHtml(typeNames[res.type]);
    output += '</span>';
    output += '</td>';
    for (var fragIdx = 0; fragIdx < res.frag.length; fragIdx++) {
      frag = res.frag[fragIdx];
      output += '<tr><td></td><td>' + frag + '</td></tr>';
    }
    output += '</tr>';
  }
  output += '</table>';

  var idx = Math.min(count, 2);
  var results = $('#searchresults');
  results.html(resultText[idx].replace(/\$num/g, count).replace(/\$query/g, query));
  results.append(output);
}

$(document).ready(function () {
  var query = trim(getURLParameter('query'));
  if (query) {
    searchFor(query, 0, 20);
  } else {
    var results = $('#results');
    results.html('<p>Sorry, there are no referenes to your query.</p>');
  }
});
