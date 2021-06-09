const fs = require('fs')
const path = require('path')

const files = fs.readdirSync('./src/hooks')
const paths = files.map(file => path.resolve(process.cwd(), './src/hooks', file))

const { NodeResolvePlugin } = require('@esbuild-plugins/node-resolve')

require('esbuild').build({
  entryPoints: paths,
  outdir: 'hooks',
  bundle: true,
  platform: 'node',
  target: 'node12',
  plugins: [
    NodeResolvePlugin({
      extensions: ['.ts', '.js'],
      onResolved: (resolved) => {
          // Mark all npm packages as external
          if (!resolved.includes('@fractal/core-ts/dist') && resolved.includes('node_modules')) {
              return {
                  external: true,
              }
          }
          return resolved
      },
    }),
  ]
})
